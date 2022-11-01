#pragma once

#include "rive_serializer.hpp"

#include "model/document.hpp"
#include "model/assets/assets.hpp"
#include "model/shapes/precomp_layer.hpp"
#include "model/shapes/rect.hpp"
#include "model/shapes/ellipse.hpp"
#include "model/shapes/polystar.hpp"
#include "model/shapes/path.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/stroke.hpp"
#include "model/shapes/image.hpp"
#include "rive_format.hpp"

namespace glaxnimate::io::rive {

namespace detail {

inline const QVariant& noop(const QVariant& v) { return v; }

} // namespace name

class RiveExporter
{

public:
    explicit RiveExporter(QIODevice* file, ImportExport* format)
        : serializer(file), format(format)
    {
        serializer.write_header(7, 0, 0);
        serializer.write_property_table({});
        write_object(TypeId::Backboard);
    }

    void write_document(model::Document* document)
    {
        write_assets(document->assets()->images.get());

        write_composition(document->main(), document->size());

        for ( const auto& comp : document->assets()->precompositions->values )
            write_composition(comp.get(), document->size());
    }

private:
    void write_assets(const model::BitmapList* assets)
    {
        for ( const auto& image : assets->values )
        {
            write_bitmap(image.get());
        }
    }

    void write_bitmap(model::Bitmap* image)
    {
        auto name = image->name.get();
        if ( name.isEmpty() )
            name = image->filename.get();

        // idk what this is used for, let's just set it to a unique value the lazy way
        Identifier asset_id = reinterpret_cast<Identifier>(image);

        auto obj = types.object(TypeId::ImageAsset);
        if ( !obj )
            return;
        object_ids[image] = next_asset++;

        obj.set("name", name);
        obj.set("width", image->width.get());
        obj.set("height", image->height.get());
        obj.set("assetId", asset_id);

        serializer.write_object(obj);

        auto data = image->image_data();
        if ( !data.isEmpty() )
        {
            auto contents = types.object(TypeId::FileAssetContents);
            if ( !contents )
                return;
            obj.set("bytes", data);
        }
    }

    bool write_object(TypeId type, const QVariantMap& props = {})
    {
        auto obj = types.object(type);
        if ( !obj )
            return false;

        for ( auto it = props.begin(); it != props.end(); ++it )
            obj.set(it.key(), *it);

        serializer.write_object(obj);

        return true;
    }

    void write_composition(model::Composition* comp, QSizeF size)
    {
        object_ids[comp] = next_artboard++;
        next_artboard_child = 1;
        animations.clear();

        if ( !write_object(TypeId::Artboard, {
            {"name", comp->name.get()},
            {"width", size.width()},
            {"height", size.height()},
            {"x", (24 + size.width()) * (next_artboard - 1)}
        }) ) return;

        for ( const auto& shape : comp->shapes )
            write_shape(shape.get(), 0);

        write_object(TypeId::LinearAnimation, {{"loopValue", 1}});
        for ( const auto& anim : animations )
        {
            write_object(TypeId::KeyedObject, {{"objectId", QVariant::fromValue(anim.first)}});
            for ( const auto& obj : anim.second )
                serializer.write_object(obj);
        }
        write_object(TypeId::StateMachine, {});
        write_object(TypeId::StateMachineLayer, {});
        write_object(TypeId::AnimationState, {{"animationId", 0}});
        write_object(TypeId::EntryState, {});
        write_object(TypeId::StateTransition, {{"stateToId", 0}});
        write_object(TypeId::AnyState, {});
        write_object(TypeId::ExitState, {});
    }

    void write_shape(model::ShapeElement* element, Identifier parent_id)
    {
        auto id = next_artboard_child++;
        object_ids[element] = id;


        if ( auto layer = element->cast<model::Layer>() )
        {
            auto object = shape_object(TypeId::Node, element, parent_id);
            write_group(object, layer, id);
        }
        else if ( auto group = element->cast<model::Group>() )
        {
            auto object = shape_object(TypeId::Shape, element, parent_id);
            write_group(object, group, id);
        }
        else if ( auto shape = element->cast<model::Rect>() )
        {
            auto object = shape_object(TypeId::Rectangle, element, parent_id);
            write_position(object, shape->position, id);
            write_property(object, "width", shape->size, id,
                [](const QVariant& v) { return QVariant::fromValue(v.toSizeF().width()); }
            );
            write_property(object, "height", shape->size, id,
                [](const QVariant& v) { return QVariant::fromValue(v.toSizeF().height()); }
            );
            write_property(object, "cornerRadiusTL", shape->rounded, id, &detail::noop);
            write_property(object, "cornerRadiusTR", shape->rounded, id, &detail::noop);
            write_property(object, "cornerRadiusBL", shape->rounded, id, &detail::noop);
            write_property(object, "cornerRadiusBR", shape->rounded, id, &detail::noop);
            serializer.write_object(object);
        }
        else if ( auto shape = element->cast<model::Ellipse>() )
        {
            auto object = shape_object(TypeId::Ellipse, element, parent_id);
            write_position(object, shape->position, id);
            write_property(object, "width", shape->size, id,
                [](const QVariant& v) { return QVariant::fromValue(v.toSizeF().width()); }
            );
            write_property(object, "height", shape->size, id,
                [](const QVariant& v) { return QVariant::fromValue(v.toSizeF().height()); }
            );
            serializer.write_object(object);
        }
        else if ( auto shape = element->cast<model::Fill>() )
        {
            auto object = shape_object(TypeId::Fill, element, parent_id);
            object.set("isVisible", shape->visible.get());
            /// \todo fillRule
            serializer.write_object(object);
            write_styler(shape, id);
        }
        else if ( auto shape = element->cast<model::Stroke>() )
        {
            auto object = shape_object(TypeId::Stroke, element, parent_id);
            write_property(object, "thickness", shape->width, id, &detail::noop);
            object.set("isVisible", shape->visible.get());
            /// \todo cap + join
            serializer.write_object(object);
            write_styler(shape, id);
        }
        /// \todo polystar path image precomplayer
        else
        {
            serializer.write_object(shape_object(TypeId::Shape, element, parent_id));
        }
    }

    Object shape_object(TypeId type_id, model::DocumentNode* shape, Identifier parent_id)
    {
        auto object = types.object(type_id);
        object.set("name", shape->name.get());
        object.set("parentId", parent_id);
        return object;
    }

    void write_group(Object& object, model::Group* group, Identifier id)
    {
        write_property(object, "opacity", group->opacity, id, &detail::noop);
        write_transform(object, group->transform.get(), id, group->local_bounding_rect(0));
        serializer.write_object(object);

        for ( const auto& shape : group->shapes )
            write_shape(shape.get(), id);
    }

    template<class T, class FuncT>
    void write_property(
        Object& object, const QString& name, const model::AnimatedProperty<T>& prop,
        Identifier object_id, const FuncT& transform)
    {
        auto rive_prop = object.type().property(name);
        if ( !rive_prop )
        {
            format->warning(QObject::tr("Unknown property %1 of %2 (%3, %4)")
                .arg(name)
                .arg(int(object.type().id))
                .arg(types.type_name(object.type().id))
                .arg(prop.object()->type_name_human())
            );
            return;
        }

        object.set(rive_prop, transform(prop.value()));

        if ( !prop.animated() )
            return;

        const ObjectType* kf_type = nullptr;
        QString attr;
        switch ( rive_prop->type )
        {
            case PropertyType::Float:
                attr = "value";
                kf_type = types.get_type(TypeId::KeyFrameDouble);
                break;
            case PropertyType::Color:
                attr = "colorValue";
                kf_type = types.get_type(TypeId::KeyFrameColor);
                break;
            default:
                break;

        }

        if ( !kf_type )
        {
            format->warning(QObject::tr("Unknown keyframe type for property %1 of %2 (%3, %4)")
                .arg(name)
                .arg(int(object.type().id))
                .arg(types.type_name(object.type().id))
                .arg(prop.object()->type_name_human())
            );
            return;
        }

        auto& keyed_object = animations[object_id];

        auto keyed_prop = types.object(TypeId::KeyedProperty);
        keyed_prop.set("propertyKey", rive_prop->id);
        keyed_object.emplace_back(std::move(keyed_prop));

        for ( const auto& kf : prop )
        {
            Object rive_kf(kf_type);
            /// \todo interpolations
            rive_kf.set("interpolationType", 1);
            rive_kf.set(attr, transform(kf.value()));
            rive_kf.set("frame", kf.time());
            keyed_object.emplace_back(std::move(rive_kf));
        }
    }

    void write_position(Object& object, const model::AnimatedProperty<QPointF>& prop, Identifier object_id)
    {
        write_property(object, "x", prop, object_id,
            [](const QVariant& v) { return QVariant::fromValue(v.toPointF().x()); }
        );
        write_property(object, "y", prop, object_id,
            [](const QVariant& v) { return QVariant::fromValue(v.toPointF().y()); }
        );
    }

    void write_transform(Object& object, model::Transform* trans, Identifier object_id, const QRectF& box)
    {
        if ( object.type().property("originX") )
        {
            write_position(object, trans->position, object_id);

            if ( box.width() > 0 )
            {
                write_property(object, "originX", trans->anchor_point, object_id,
                    [&box](const QVariant& v) {
                        return QVariant::fromValue(
                            (v.toPointF().x() - box.left()) / box.width()
                        );
                    }
                );
            }

            if ( box.height() > 0 )
            {
                write_property(object, "originY", trans->anchor_point, object_id,
                    [&box](const QVariant& v) {
                        return QVariant::fromValue(
                            (v.toPointF().y() - box.top()) / box.height()
                        );
                    }
                );
            }
        }
        else
        {
            /// \todo Handle animated anchor point
            auto anchor = trans->anchor_point.get();
            write_property(object, "x", trans->position, object_id,
                [anchor](const QVariant& v) { return QVariant::fromValue(v.toPointF().x() - anchor.x()); }
            );
            write_property(object, "y", trans->position, object_id,
                [anchor](const QVariant& v) { return QVariant::fromValue(v.toPointF().y() - anchor.y()); }
            );
        }

        write_property(object, "rotation", trans->rotation, object_id, &detail::noop);

        write_property(object, "scaleX", trans->scale, object_id,
            [](const QVariant& v) { return QVariant::fromValue(v.value<QVector2D>().x()); }
        );
        write_property(object, "scaleY", trans->scale, object_id,
            [](const QVariant& v) { return QVariant::fromValue(v.value<QVector2D>().x()); }
        );
    }

    void write_styler(model::Styler* shape, Identifier object_id)
    {
        auto use = shape->use.get();
        auto id = next_artboard_child++;

        if ( auto grad = use->cast<model::Gradient>() )
        {
            auto object = shape_object(
                grad->type.get() == model::Gradient::Radial ? TypeId::RadialGradient : TypeId::LinearGradient,
                grad, object_id
            );
            write_property(object, "opacity", shape->color, id, &detail::noop);

            serializer.write_object(object);
            /// \todo finish
        }
        else if ( auto col = use->cast<model::NamedColor>() )
        {
            auto object = shape_object(TypeId::SolidColor, col, object_id);
            write_property(object, "colorValue", col->color, id, &detail::noop);
            serializer.write_object(object);
        }
        else
        {
            auto object = shape_object(TypeId::SolidColor, shape, object_id);
            write_property(object, "colorValue", shape->color, id, &detail::noop);
            serializer.write_object(object);
        }
    }

    Identifier next_asset = 0;
    Identifier next_artboard = 0;
    Identifier next_artboard_child = 0;
    std::unordered_map<model::DocumentNode*, Identifier> object_ids;
    RiveSerializer serializer;
    ImportExport* format;
    std::unordered_map<Identifier, std::vector<Object>> animations;
    TypeSystem types;
};

} // namespace glaxnimate::io::rive
