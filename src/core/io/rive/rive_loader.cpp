/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "rive_loader.hpp"

#include "io/animated_properties.hpp"
#include "model/assets/assets.hpp"
#include "model/shapes/precomp_layer.hpp"
#include "model/shapes/rect.hpp"
#include "model/shapes/ellipse.hpp"
#include "model/shapes/polystar.hpp"
#include "model/shapes/path.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/stroke.hpp"
#include "model/shapes/image.hpp"

using namespace glaxnimate;
using namespace glaxnimate::io;
using namespace glaxnimate::io::rive;

namespace {

struct Artboard
{
    Artboard() = default;

    Artboard(Object* first, Object* last)
        : object(first),
            children(object),
            child_count(last - first + 1)
    {}

    Object* operator->() const { return object; }

    Object* object = nullptr;
    Object* children = nullptr;
    Identifier child_count = 0;
    VarUint timeline_duration = 0;
    VarUint keyframe_timeline_duration = 0;
    model::Composition* comp = nullptr;
    QSizeF size;
};

template<class T> T load_property_get_keyframe(const detail::JoinedPropertyKeyframe& kf, std::size_t index);
template<> Float32 load_property_get_keyframe<Float32>(const detail::JoinedPropertyKeyframe& kf, std::size_t index)
{
    return kf.values[index].vector()[0];
}
template<> VarUint load_property_get_keyframe<VarUint>(const detail::JoinedPropertyKeyframe& kf, std::size_t index)
{
    return kf.values[index].vector()[0];
}
template<> QColor load_property_get_keyframe<QColor>(const detail::JoinedPropertyKeyframe& kf, std::size_t index)
{
    return kf.values[index].color();
}

template<class... T, class PropT, class Func, std::size_t... Ind, std::size_t N>
void load_property_impl(Object* rive, PropT& property, const detail::AnimatedProperties& animations,
                    const std::array<const char*, N>& names, T... defvals, const Func& value_func, std::index_sequence<Ind...>)
{
    property.set(value_func(rive->get<T>(names[Ind], defvals)...));

    for ( const auto& kf : animations.joined(std::vector<QString>(names.begin(), names.end())) )
        property.set_keyframe(kf.time, value_func(load_property_get_keyframe<T>(kf, Ind)...))->set_transition(kf.transition);

}

template<class... T, class PropT, class Func>
void load_property(Object* rive, PropT& property, const detail::AnimatedProperties& animations,
                    const std::array<const char*, sizeof...(T)>& names, T... defvals, const Func& value_func)
{
    load_property_impl<T...>(rive, property, animations, names, defvals..., value_func, std::index_sequence_for<T...>{});
}

template<std::size_t RetInd, class... T, class PropT, class Func, std::size_t... Ind, std::size_t N>
void load_property_vector_impl(Object* rive, PropT& property, const detail::AnimatedProperties& animations,
                    const std::array<const char*, N>& names, T... defvals, const Func& value_func, std::index_sequence<Ind...>)
{
    property.set(
        std::get<RetInd>(
            value_func(rive->get<T>(names[Ind], defvals)...)
        )
    );

    for ( const auto& kf : animations.joined(std::vector<QString>(names.begin(), names.end())) )
        property.set_keyframe(kf.time,
            std::get<RetInd>(value_func(load_property_get_keyframe<T>(kf, Ind)...))
        )->set_transition(kf.transition);

}


template<class T>
void expand(T...) {}

template<class... T, class... PropT, class Func, std::size_t... Ind, std::size_t... PropInd, std::size_t N>
void load_properties_impl(Object* rive, const std::tuple<PropT...>& properties, const detail::AnimatedProperties& animations,
                    const std::array<const char*, N>& names, T... defvals, const Func& value_func,
                    std::index_sequence<Ind...> ind, std::index_sequence<PropInd...>)
{
    expand(
        (load_property_vector_impl<PropInd, T...>(
            rive, *std::get<PropInd>(properties), animations, names, defvals..., value_func, ind
        ), 0) ...
    );
}

template<class... T, class... PropT, class Func>
void load_properties(
    Object* rive, std::tuple<PropT...> properties, const detail::AnimatedProperties& animations,
    const std::array<const char*, sizeof...(T)>& names, T... defvals, const Func& value_func)
{
    load_properties_impl<T...>(
        rive, properties, animations, names, defvals..., value_func,
        std::index_sequence_for<T...>{}, std::index_sequence_for<PropT...>{});
}

template<class T, class PropT>
void load_property(Object* rive, PropT& property, const detail::AnimatedProperties& animations, const char* name, T defval = {})
{
    property.set(rive->get<T>(name, defval));

    for ( const auto& kf : animations.joined({name}) )
        property.set_keyframe(kf.time, load_property_get_keyframe<T>(kf, 0))->set_transition(kf.transition);
}

QPointF make_point(Float32 x, Float32 y)
{
    return QPointF(x, y);
}

struct Asset
{
    Object* object = nullptr;
    model::Asset* asset = nullptr;
};

struct LoadCotext
{
    void new_artboard(Object* object)
    {
        artboards[object] = Artboard(object, &objects.back());
        artboard = &artboards[object];
        artboards_id.push_back(artboard);
        artboard->comp = document->assets()->compositions->values.insert(std::make_unique<model::Composition>(document));
        artboard->size = QSizeF(
            object->get<Float32>("width"),
            object->get<Float32>("height")
        );
    }

    Object* artboard_child(Identifier id) const
    {
        if ( artboard && id < artboard->child_count )
            return artboard->children + id;
        return nullptr;
    }

    LoadCotext(RiveFormat* format, model::Document* document)
        : document(document), format(format)
    {
        main = document->assets()->compositions->values.insert(std::make_unique<model::Composition>(document));
    }

    void preprocess_object(Object* object)
    {
        if ( object->type().id == TypeId::Artboard )
        {
            new_artboard(object);
        }
        else if ( object->type().id == TypeId::KeyedObject )
        {
            if ( !artboard )
            {
                format->warning(QObject::tr("Unexpected Keyed Object"));
                return;
            }
            auto id = object->get<Identifier>("objectId", artboard->child_count);
            keyed_object = artboard_child(id);
            keyed_property = nullptr;
            if ( !keyed_object )
            {
                format->warning(QObject::tr("Invalid Keyed Object id %1").arg(id));
                return;
            }
        }
        else if ( object->type().id == TypeId::KeyedProperty )
        {
            if ( !keyed_object )
            {
                format->warning(QObject::tr("Unexpected Keyed Property"));
                return;
            }

            auto id = object->get<Identifier>("propertyKey");
            auto prop = keyed_object->type().property(id);

            if ( !prop )
            {
                format->warning(QObject::tr("Unknown Keyed Property id %1").arg(id));
                return;
            }

            keyed_object->animations().push_back({prop, {}});
            keyed_property = &keyed_object->animations().back();
        }
        else if ( object->type().id == TypeId::LinearAnimation )
        {
            if ( !artboard )
            {
                format->warning(QObject::tr("Unexpected Animation"));
                return;
            }

            auto duration = object->get<VarUint>("duration");
            if ( duration > artboard->timeline_duration )
                artboard->timeline_duration = duration;
        }
        else if ( object->type().id == TypeId::ImageAsset )
        {
            assets.push_back({object, load_image_asset(object)});
        }
        else if ( object->type().id == TypeId::FileAssetContents )
        {
            if ( assets.empty() )
            {
                format->warning(QObject::tr("Unexpected Asset Contents"));
                return;
            }

            auto data = object->get<QByteArray>("bytes");
            if ( data.isEmpty() )
                return;

            if ( auto img = qobject_cast<model::Bitmap*>(assets.back().asset) )
            {
                if ( !img->from_raw_data(data) )
                    format->warning(QObject::tr("Invalid Image Data"));
            }
        }
        else if ( object->has_type(TypeId::Asset) )
        {
            assets.push_back({object, nullptr});
        }
        else if ( object->has_type(TypeId::KeyFrame) )
        {
            if ( !keyed_property )
            {
                format->warning(QObject::tr("Unexpected Keyframe"));
                return;
            }

            auto frame = object->get<VarUint>("duration");
            if ( frame > artboard->keyframe_timeline_duration )
                artboard->keyframe_timeline_duration = frame;

            keyed_property->keyframes.push_back(object);
        }
        else if ( object->has("parentId") )
        {
            auto parent_id = object->get<Identifier>("parentId");
            auto parent = artboard_child(parent_id);
            if ( !parent )
                format->warning(QObject::tr("Could not find parent with id %1").arg(parent_id));
            else
                parent->children().push_back(object);
        }
    }

    void process_object(Object* object)
    {
        if ( object->type().id == TypeId::Artboard )
        {
            process_artboard(object);
        }
    }

    void process_artboard(Object* object)
    {
        const auto& artboard = artboards.at(object);

        artboard.comp->name.set(object->get<QString>("name"));
        add_shapes(object, artboard.comp->shapes);

        auto precomp_layer = std::make_unique<model::PreCompLayer>(document);
        precomp_layer->name.set(artboard.comp->name.get());
        precomp_layer->size.set(artboard.size.toSize());
        detail::AnimatedProperties animations = load_animations(object);
        load_transform(object, precomp_layer->transform.get(), animations, QRectF(QPointF(0, 0), artboard.size));
        precomp_layer->opacity.set(object->get<Float32>("opacity", 1));
        precomp_layer->composition.set(artboard.comp);

        float last_frame = artboard.timeline_duration == 0 ? artboard.keyframe_timeline_duration : artboard.timeline_duration;
        main->animation->last_frame.set(qMax(main->animation->last_frame.get(), last_frame));

        if ( document->assets()->compositions->values.size() == 1 )
        {
            main->width.set(precomp_layer->size.get().width());
            main->height.set(precomp_layer->size.get().height());
        }

        main->shapes.insert(std::move(precomp_layer));
    }

    void add_shapes(Object* parent, model::ObjectListProperty<model::ShapeElement>& prop)
    {

        std::vector<std::unique_ptr<model::ShapeElement>> shapes;

        for ( Object* child : parent->children() )
        {
            if ( child == parent )
            {
                format->error(QObject::tr("Parent circular reference detected"));
                continue;
            }

            auto shape = load_shape(child);
            if ( shape )
            {
                if ( child->has_type(TypeId::Node) )
                    shapes.emplace_back(std::move(shape));
                else
                    prop.insert(std::move(shape));
            }
        }

        for ( auto it = shapes.rbegin(); it != shapes.rend(); ++it )
            prop.insert(std::move(*it));
    }

    std::unique_ptr<model::Layer> load_shape_layer(Object* shape, const detail::AnimatedProperties& animations)
    {
        auto layer = std::make_unique<model::Layer>(document);
        load_shape_group(shape, layer.get(), animations);
        return layer;
    }

    void load_transform(Object* rive, model::Transform* transform, const detail::AnimatedProperties& animations, const QRectF& bbox)
    {
        load_property<Float32, Float32>(rive, transform->position, animations, {"x", "y"}, 0, 0, &make_point);

        if ( rive->type().property("originX") )
        {
            load_property<Float32, Float32>(rive, transform->anchor_point, animations, {"originX", "originY"}, 0.5, 0.5,
                [&bbox](Float32 ox, Float32 oy){
                    return QPointF(
                        math::lerp(bbox.left(), bbox.right(), ox),
                        math::lerp(bbox.top(), bbox.bottom(), oy)
                    );
                }
            );
        }

        /*load_properties<Float32, Float32, Float32, Float32>(
            rive,
            std::make_tuple(&transform->position, &transform->anchor_point),
            animations,
            {"x", "y", "originX", "originY"},
            0, 0, 0.5, 0.5,
            [&bbox] ( Float32 x, Float32 y, Float32 ox, Float32 oy ) {
                QPointF anchor(
                    math::lerp(bbox.left(), bbox.right(), ox),
                    math::lerp(bbox.top(), bbox.bottom(), oy)
                );
                return std::make_tuple(QPointF(x, y) - anchor, anchor);
            }
        );*/
        load_property<Float32>(rive, transform->rotation, animations, "rotation");
        load_property<Float32, Float32>(rive, transform->scale, animations, {"scaleX", "scaleX"}, 1, 1, [](Float32 x, Float32 y){
            return QVector2D(x, y);
        });
    }

    void load_shape_group(Object* shape, model::Group* group, const detail::AnimatedProperties& animations)
    {
        load_property<Float32>(shape, group->opacity, animations, "opacity", 1);
        group->name.set(shape->get<QString>("name"));
        add_shapes(shape, group->shapes);
        auto box = group->local_bounding_rect(0);
        load_transform(shape, group->transform.get(), animations, box);
    }

    std::unique_ptr<model::ShapeElement> load_shape(Object* object)
    {
        detail::AnimatedProperties animations = load_animations(object);

        switch ( object->type().id )
        {
            case TypeId::Shape:
            case TypeId::Node:
                return load_shape_layer(object, animations);
            case TypeId::Rectangle:
                return load_rectangle(object, animations);
            case TypeId::Ellipse:
                return load_ellipse(object, animations);
            case TypeId::Fill:
                return load_fill(object, animations);
            case TypeId::Stroke:
                return load_stroke(object, animations);
            case TypeId::Polygon:
                return load_polygon(object, animations, model::PolyStar::Polygon);
            case TypeId::Star:
                return load_polygon(object, animations, model::PolyStar::Star);
            case TypeId::Triangle:
                return load_triangle(object, animations);
            case TypeId::PointsPath:
                return load_path(object, animations);
            case TypeId::NestedArtboard:
                return load_precomp(object, animations);
            case TypeId::Image:
                return load_image(object, animations);
            case TypeId::TrimPath:
            case TypeId::Bone:
            case TypeId::RootBone:
            case TypeId::ClippingShape:
            case TypeId::Text:
                /// \todo
            default:
                return {};
        }
    }

    std::unique_ptr<model::Group> load_rectangle(Object* object, const detail::AnimatedProperties& animations)
    {
        auto group = std::make_unique<model::Group>(document);
        auto shape = std::make_unique<model::Rect>(document);
        shape->name.set(object->get<QString>("name"));

        load_property<Float32, Float32, Float32, Float32>(object, shape->rounded, animations,
            {"cornerRadiusTL", "cornerRadiusBL", "cornerRadiusBR", "cornerRadiusTR"},
            0, 0, 0, 0,
            [](Float32 tl, Float32 bl, Float32 br, Float32 tr){
                return (tl + bl + br + tr) / 4;
            }
        );

        load_property<Float32, Float32>(object, shape->size, animations, {"width", "height"}, 0, 0, [](Float32 x, Float32 y){
            return QSizeF(x, y);
        });

        group->shapes.insert(std::move(shape));
        load_shape_group(object, group.get(), animations);
        return group;
    }

    std::unique_ptr<model::Group> load_ellipse(Object* object, const detail::AnimatedProperties& animations)
    {
        auto group = std::make_unique<model::Group>(document);
        auto shape = std::make_unique<model::Ellipse>(document);
        shape->name.set(object->get<QString>("name"));


        load_property<Float32, Float32>(object, shape->size, animations, {"width", "height"}, 0, 0, [](Float32 x, Float32 y){
            return QSizeF(x, y);
        });

        group->shapes.insert(std::move(shape));
        load_shape_group(object, group.get(), animations);
        return group;
    }

    std::unique_ptr<model::Fill> load_fill(Object* object, const detail::AnimatedProperties& animations)
    {
        auto shape = std::make_unique<model::Fill>(document);
        load_styler(object, shape.get(), animations);
        /// \todo fillRule
        return shape;
    }

    std::unique_ptr<model::Stroke> load_stroke(Object* object, const detail::AnimatedProperties& animations)
    {
        auto shape = std::make_unique<model::Stroke>(document);
        load_styler(object, shape.get(), animations);
        load_property<Float32>(object, shape->width, animations, "thickness");
        /// \todo cap + join
        return shape;
    }

    void load_styler(Object* object, model::Styler* shape, const detail::AnimatedProperties& animations)
    {
        shape->name.set(object->get<QString>("name"));
        shape->visible.set(object->get<bool>("isVisible", true));
        load_property<Float32>(object, shape->opacity, animations, "opacity", 1);

        for ( const auto& child : object->children() )
        {
            if ( child->type().id == TypeId::SolidColor )
                load_property<QColor>(child, shape->color, load_animations(child), "colorValue", QColor("#747474"));
            else if ( child->type().id == TypeId::LinearGradient )
                shape->use.set(load_gradient(child, model::Gradient::Linear));
            else if ( child->type().id == TypeId::RadialGradient )
                shape->use.set(load_gradient(child, model::Gradient::Radial));
        }
    }

    model::Gradient* load_gradient(Object* object, model::Gradient::GradientType type)
    {
        auto colors = std::make_unique<glaxnimate::model::GradientColors>(document);
        colors->name.set(object->get<QString>("name"));
        auto colors_ptr = colors.get();
        document->assets()->gradient_colors->values.insert(std::move(colors));

        auto gradient = std::make_unique<glaxnimate::model::Gradient>(document);
        gradient->name.set(object->get<QString>("name"));
        gradient->colors.set(colors_ptr);
        gradient->type.set(type);

        auto animations = load_animations(object);
        load_property<Float32, Float32>(object, gradient->start_point, animations, {"startX", "startY"}, 0, 0, &make_point);
        load_property<Float32, Float32>(object, gradient->end_point, animations, {"endX", "endY"}, 0, 0, &make_point);

        /// \todo color animations
        QGradientStops stops;
        for ( const auto& child : object->children() )
        {
            if ( child->type().id == TypeId::GradientStop )
            {
                stops.push_back({
                    child->get<Float32>("position"),
                    child->get<QColor>("colorValue"),
                });
            }
        }
        colors_ptr->colors.set(stops);

        auto ptr = gradient.get();
        document->assets()->gradients->values.insert(std::move(gradient));
        return ptr;
    }

    detail::AnimatedProperties load_animations(Object* object)
    {
        using namespace glaxnimate::io::detail;

        AnimatedProperties props;
        for ( const auto& anim : object->animations() )
        {
            AnimatedProperty& prop = props.properties[anim.property->name];
            for ( auto kf : anim.keyframes )
            {
                model::KeyframeTransition transition; /// \todo
                prop.keyframes.push_back({
                    kf->get<Float32>("frame", 0),
                    ValueVariant(kf->get_variant("value")),
                    transition
                });
            }
        }
        return props;
    }

    std::unique_ptr<model::Group> load_polygon(Object* object, const detail::AnimatedProperties& animations, model::PolyStar::StarType type)
    {
        auto group = std::make_unique<model::Group>(document);
        load_shape_group(object, group.get(), animations);

        auto shape = std::make_unique<model::PolyStar>(document);
        shape->name.set(object->get<QString>("name"));
        shape->type.set(type);
        /// \todo cornerRadius
        load_property<VarUint>(object, shape->points, animations, "points", 5);
        shape->outer_radius.set(100);

        load_property<Float32>(object, shape->inner_radius, animations, {"innerRadius"}, 0.5, [](Float32 pc){
            return pc * 100;
        });

        load_property<Float32>(object, shape->points, animations, "points", 5);


        load_property<Float32, Float32, Float32, Float32>(object, group->transform->scale, animations,
            {"scaleX", "scaleY", "width", "height"},
            1, 1, 0, 0, [](Float32 sx, Float32 sy, Float32 w, Float32 h){
            return QVector2D(w / 200 * sx, h / 200 * sy);
        });

        group->shapes.insert(std::move(shape));
        return group;
    }

    std::unique_ptr<model::Group> load_triangle(Object* object, const detail::AnimatedProperties& animations)
    {
        auto group = std::make_unique<model::Group>(document);
        auto shape = std::make_unique<model::Path>(document);
        shape->name.set(object->get<QString>("name"));


        load_property<Float32, Float32>(object, shape->shape, animations, {"width", "height"}, 0, 0, [](Float32 w, Float32 h){
            math::bezier::Bezier path;
            path.add_point({-w/2, h/2});
            path.add_point({0, -h/2});
            path.add_point({w/2, h/2});
            path.close();
            return path;
        });

        group->shapes.insert(std::move(shape));
        load_shape_group(object, group.get(), animations);
        return group;
    }

    std::unique_ptr<model::Path> load_path(Object* object, const detail::AnimatedProperties& animations)
    {
        auto shape = std::make_unique<model::Path>(document);
        shape->name.set(object->get<QString>("name"));
        bool closed = object->get<bool>("isClosed");
        shape->closed.set(closed);

        math::bezier::Bezier bez;
        for ( const auto& child : object->children() )
        {
            math::bezier::Point p;
            p.pos = QPointF(child->get<Float32>("x", 0), child->get<Float32>("y", 0));
            if ( child->type().id == TypeId::CubicMirroredVertex )
            {
                p.type = math::bezier::Symmetrical;
                auto tangent = math::from_polar<QPointF>(
                    child->get<Float32>("distance"),
                    child->get<Float32>("rotation")
                );
                p.tan_in = p.pos - tangent;
                p.tan_out = p.pos + tangent;
            }
            else if ( child->type().id == TypeId::CubicAsymmetricVertex )
            {
                p.type = math::bezier::Smooth;
                p.tan_in = p.pos - math::from_polar<QPointF>(
                    child->get<Float32>("inDistance"),
                    child->get<Float32>("rotation")
                );
                p.tan_out = p.pos + math::from_polar<QPointF>(
                    child->get<Float32>("outDistance"),
                    child->get<Float32>("rotation")
                );
            }
            else if ( child->type().id == TypeId::CubicDetachedVertex )
            {
                p.type = math::bezier::Corner;
                p.tan_in = p.pos + math::from_polar<QPointF>(
                    child->get<Float32>("inDistance"),
                    child->get<Float32>("inRotation")
                );
                p.tan_out = p.pos + math::from_polar<QPointF>(
                    child->get<Float32>("outDistance"),
                    child->get<Float32>("outRotation")
                );
            }
            else if ( child->type().id == TypeId::StraightVertex )
            {
                p.type = math::bezier::Corner;
                p.tan_in = p.tan_out = p.pos;
            }
            else
            {
                continue;
            }

            bez.push_back(p);
        }

        bez.set_closed(closed);

        /// \todo animation
        Q_UNUSED(animations);
        shape->shape.set(bez);

        return shape;
    }

    std::unique_ptr<model::PreCompLayer> load_precomp(Object* object, const detail::AnimatedProperties& animations)
    {
        auto shape = std::make_unique<model::PreCompLayer>(document);
        shape->name.set(object->get<QString>("name"));
        load_property<Float32>(object, shape->opacity, animations, "opacity", 1);

        QRectF box;

        if ( object->has("artboardId") )
        {
            auto id = object->get<VarUint>("artboardId");
            shape->size.set(artboards_id[id]->size);
            shape->composition.set(artboards_id[id]->comp);
            box.setSize(artboards_id[id]->size);
        }

        load_transform(object, shape->transform.get(), animations, box);
        return shape;
    }

    model::Bitmap* load_image_asset(Object* object)
    {
        auto image = std::make_unique<glaxnimate::model::Bitmap>(document);
        image->filename.set(object->get<QString>("name"));
        image->width.set(object->get<Float32>("width"));
        image->height.set(object->get<Float32>("height"));
        auto ptr = image.get();
        document->assets()->images->values.insert(std::move(image));
        return ptr;
    }

    std::unique_ptr<model::Image> load_image(Object* object, const detail::AnimatedProperties& animations)
    {
        auto shape = std::make_unique<model::Image>(document);
        shape->name.set(object->get<QString>("name"));
        auto id = object->get<VarUint>("assetId");
        QSizeF size;
        if ( auto bmp = qobject_cast<model::Bitmap*>(assets[id].asset) )
        {
            size = bmp->size();
            shape->transform->anchor_point.set(QPointF(
                bmp->width.get() / 2.,
                bmp->height.get() / 2.
            ));
            shape->image.set(bmp);
        }
        load_transform(object, shape->transform.get(), animations, {QPointF(0, 0), size});

        return shape;
    }

    model::Document* document = nullptr;
    std::map<Object*, Artboard> artboards;
    std::vector<Object> objects;
    Artboard* artboard = nullptr;
    Object* keyed_object = nullptr;
    PropertyAnimation* keyed_property = nullptr;
    RiveFormat* format = nullptr;
    std::vector<Artboard*> artboards_id;
    std::vector<Asset> assets;
    model::Composition* main = nullptr;
};

} // namespace

RiveLoader::RiveLoader(BinaryInputStream& stream, RiveFormat* format)
    : stream(stream),
    format(format)
{
    extra_props = read_property_table();
    QObject::connect(&types, &TypeSystem::type_not_found, [format](int type){
        format->error(QObject::tr("Unknown object of type %1").arg(int(type)));
    });

    if ( stream.has_error() )
        format->error(QObject::tr("Could not read property table"));

}

std::vector<Object> RiveLoader::load_object_list()
{
    if ( stream.has_error() )
        return {};

    std::vector<Object> objects;
    while ( !stream.has_error() && !stream.eof() )
        objects.emplace_back(read_object());
    return objects;
}

bool RiveLoader::load_document(model::Document* document)
{
    if ( stream.has_error() )
        return false;

    LoadCotext context(format, document);

    while ( !stream.has_error() && !stream.eof() )
        if ( auto obj = read_object() )
            context.objects.emplace_back(std::move(obj));

    for ( auto& object : context.objects )
        context.preprocess_object(&object);


    for ( auto& object : context.objects )
        context.process_object(&object);

    return true;
}

Object RiveLoader::read_object()
{
    auto type_id = TypeId(stream.read_uint_leb128());
    if ( stream.has_error() )
    {
        format->error(QObject::tr("Could not load object type ID"));
        return {};
    }

    Object obj = types.object(type_id);

    if ( !obj )
        return {};

    while ( true )
    {
        Identifier prop_id = stream.read_uint_leb128();
        if ( stream.has_error() )
        {
            format->error(QObject::tr("Could not load property ID in %1 (%2)")
                .arg(int(type_id)).arg(obj.definition()->name));
            return {};
        }

        if ( prop_id == 0 )
            break;

        auto prop_def = obj.type().property(prop_id);
        if ( !prop_def )
        {
            auto unknown_it = extra_props.find(prop_id);
            if ( unknown_it == extra_props.end() )
            {
                format->error(QObject::tr("Unknown property %1 of %2 (%3)")
                    .arg(prop_id).arg(int(type_id)).arg(obj.definition()->name));
                return {};
            }
            else
            {
                format->warning(QObject::tr("Skipping unknown property %1 of %2 (%3)")
                        .arg(prop_id).arg(int(type_id)).arg(obj.definition()->name));
            }
        }
        else
        {
            obj.set(prop_def, read_property_value(prop_def->type));
            if ( stream.has_error() )
            {
                format->error(QObject::tr("Error loading property %1 (%2) of %3 (%4)")
                    .arg(prop_id).arg(prop_def->name).arg(int(type_id)).arg(obj.definition()->name));
                return {};
            }
        }
    }

    return obj;
}


QVariant RiveLoader::read_property_value(PropertyType type)
{
    switch ( type )
    {
        case PropertyType::Bool:
            return bool(stream.next());
        case PropertyType::Bytes:
            return read_raw_string();
        case PropertyType::String:
            return read_string_utf8();
        case PropertyType::VarUint:
            return QVariant::fromValue(stream.read_uint_leb128());
        case PropertyType::Float:
            return stream.read_float32_le();
        case PropertyType::Color:
            return QColor::fromRgba(stream.read_uint32_le());
    }

    return {};
}


PropertyTable RiveLoader::read_property_table()
{
    std::vector<VarUint> props;
    while ( true )
    {
        VarUint id = stream.read_uint_leb128();
        if ( stream.has_error() )
            return {};

        if ( id == 0 )
            break;

        props.push_back(id);
    }

    quint32 current_int = 0;
    quint32 bit = 8;

    PropertyTable table;

    for ( auto id : props )
    {
        if ( bit == 8 )
        {
            current_int = stream.read_uint32_le();
            if ( stream.has_error() )
                return {};
            bit = 0;
        }

        int type = (current_int >> bit) & 3;
        if ( type == 0 )
            table[id] = PropertyType::VarUint;
        else if ( type == 1 )
            table[id] = PropertyType::String;
        else if ( type == 2 )
            table[id] = PropertyType::Float;
        else if ( type == 3 )
            table[id] = PropertyType::Color;

        bit += 2;
    }

    return table;
}

void RiveLoader::skip_value(glaxnimate::io::rive::PropertyType type)
{
    switch ( type )
    {
        case PropertyType::Bool:
        case PropertyType::VarUint:
            stream.read_uint_leb128();
            break;
        case PropertyType::Bytes:
        case PropertyType::String:
            read_raw_string();
            break;
        case PropertyType::Float:
            stream.read_float32_le();
            break;
        case PropertyType::Color:
            stream.read_uint32_le();
            break;
    }
}

const PropertyTable& RiveLoader::extra_properties() const
{
    return extra_props;
}

QByteArray RiveLoader::read_raw_string()
{
    auto size = stream.read_uint_leb128();
    if ( stream.has_error() )
        return {};

    return stream.read(size);
}

QString RiveLoader::read_string_utf8()
{
    return QString::fromUtf8(read_raw_string());
}
