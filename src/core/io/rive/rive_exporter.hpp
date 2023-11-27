/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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

inline const QVariant& noop(const QVariant& v, model::FrameTime) { return v; }

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


        for ( const auto& comp : document->assets()->compositions->values )
            write_composition(comp.get(), comp->size());
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

        obj.set("name"_qs, name);
        obj.set("width"_qs, image->width.get());
        obj.set("height"_qs, image->height.get());
        obj.set("assetId"_qs, asset_id);

        serializer.write_object(obj);

        auto data = image->image_data();
        if ( !data.isEmpty() )
        {
            auto contents = types.object(TypeId::FileAssetContents);
            if ( !contents )
                return;
            obj.set("bytes"_qs, data);
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
            {"name"_qs, comp->name.get()},
            {"width"_qs, size.width()},
            {"height"_qs, size.height()},
            {"x"_qs, (24 + size.width()) * (next_artboard - 1)}
        }) ) return;

        for ( const auto& shape : comp->shapes )
            write_shape(shape.get(), 0);

        write_object(TypeId::LinearAnimation, {{"loopValue"_qs, 1}});
        for ( const auto& anim : animations )
        {
            write_object(TypeId::KeyedObject, {{"objectId"_qs, QVariant::fromValue(anim.first)}});
            for ( const auto& obj : anim.second )
                serializer.write_object(obj);
        }
        write_object(TypeId::StateMachine, {});
        write_object(TypeId::StateMachineLayer, {});
        write_object(TypeId::AnimationState, {{"animationId"_qs, 0}});
        write_object(TypeId::EntryState, {});
        write_object(TypeId::StateTransition, {{"stateToId"_qs, 0}});
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
            write_rect(shape, id, parent_id);
        }
        else if ( auto shape = element->cast<model::Ellipse>() )
        {
            write_ellipse(shape, id, parent_id);
        }
        else if ( auto shape = element->cast<model::PolyStar>() )
        {
            write_polystar(shape, id, parent_id);
        }
        else if ( auto shape = element->cast<model::Fill>() )
        {
            auto object = shape_object(TypeId::Fill, element, parent_id);
            object.set("isVisible"_qs, shape->visible.get());
            /// \todo fillRule
            serializer.write_object(object);
            write_styler(shape, id);
        }
        else if ( auto shape = element->cast<model::Stroke>() )
        {
            auto object = shape_object(TypeId::Stroke, element, parent_id);
            write_property(object, "thickness"_qs, shape->width, id, &detail::noop);
            object.set("isVisible"_qs, shape->visible.get());
            /// \todo cap + join
            serializer.write_object(object);
            write_styler(shape, id);
        }
        else if ( auto shape = element->cast<model::Image>() )
        {
            auto object = shape_object(TypeId::Image, element, parent_id);
            write_transform(object, shape->transform.get(), id, shape->local_bounding_rect(0));
            auto asset_id = object_ids.find(shape->image.get());
            if ( asset_id != object_ids.end() )
                object.set("assetId"_qs, asset_id->second);
            serializer.write_object(object);
        }
        else if ( auto shape = element->cast<model::PreCompLayer>() )
        {
            write_precomp_layer(shape, id, parent_id);
        }
        else if ( auto shape = element->cast<model::Path>() )
        {
            write_path(shape, id, parent_id);
        }
        else
        {
            serializer.write_object(shape_object(TypeId::Shape, element, parent_id));
        }
    }

    void write_rect(model::Rect* shape, Identifier id, Identifier parent_id)
    {
        auto object = shape_object(TypeId::Rectangle, shape, parent_id);
        write_position(object, shape->position, id);
        write_property(object, "width"_qs, shape->size, id,
            [](const QVariant& v, model::FrameTime) { return QVariant::fromValue(v.toSizeF().width()); }
        );
        write_property(object, "height"_qs, shape->size, id,
            [](const QVariant& v, model::FrameTime) { return QVariant::fromValue(v.toSizeF().height()); }
        );
        write_property(object, "cornerRadiusTL"_qs, shape->rounded, id, &detail::noop);
        write_property(object, "cornerRadiusTR"_qs, shape->rounded, id, &detail::noop);
        write_property(object, "cornerRadiusBL"_qs, shape->rounded, id, &detail::noop);
        write_property(object, "cornerRadiusBR"_qs, shape->rounded, id, &detail::noop);
        serializer.write_object(object);
    }

    void write_ellipse(model::Ellipse* shape, Identifier id, Identifier parent_id)
    {
        auto object = shape_object(TypeId::Ellipse, shape, parent_id);
        write_position(object, shape->position, id);
        write_property(object, "width"_qs, shape->size, id,
            [](const QVariant& v, model::FrameTime) { return QVariant::fromValue(v.toSizeF().width()); }
        );
        write_property(object, "height"_qs, shape->size, id,
            [](const QVariant& v, model::FrameTime) { return QVariant::fromValue(v.toSizeF().height()); }
        );
        serializer.write_object(object);
    }

    void write_polystar(model::PolyStar* shape, Identifier id, Identifier parent_id)
    {
        auto type = shape->type.get() == model::PolyStar::Star ? TypeId::Star : TypeId::Polygon;
        auto object = shape_object(type, shape, parent_id);
        /// \todo cornerRadius
        write_position(object, shape->position, id);

        write_property(object, "points"_qs, shape->points, id, &detail::noop);
        write_property(object, "width"_qs, shape->outer_radius, id, &detail::noop);
        write_property(object, "height"_qs, shape->outer_radius, id, &detail::noop);

        if ( type == TypeId::Star )
        {
            write_property(object, "innerRadius"_qs, shape->inner_radius, id,
                [shape](const QVariant& v, model::FrameTime t) {
                    auto outer = shape->outer_radius.get_at(t);
                    return QVariant::fromValue(qFuzzyIsNull(outer) ? 0 : v.toDouble() / outer);
                }
            );
        }

        serializer.write_object(object);
    }

    void write_precomp_layer(model::PreCompLayer* shape, Identifier id, Identifier parent_id)
    {
        auto object = shape_object(TypeId::Rectangle, shape, parent_id);
        write_transform(object, shape->transform.get(), id, shape->local_bounding_rect(0));
        write_property(object, "opacity"_qs, shape->opacity, id, &detail::noop);
        if ( auto comp = shape->composition.get() )
        {
            Identifier comp_index = 1;
            for ( const auto& declared_comp : shape->document()->assets()->compositions->values )
            {
                if ( declared_comp.get() == comp )
                    break;
                comp_index++;
            }
            object.set("artboardId"_qs, comp_index);
        }

        serializer.write_object(object);
    }

    void write_path(model::Path* shape, Identifier id, Identifier parent_id)
    {
        auto object = shape_object(TypeId::PointsPath, shape, parent_id);
        object.set("isClosed"_qs, shape->closed.get());
        serializer.write_object(object);

        auto first_point_id = next_artboard_child;
        auto animated = shape->shape.keyframe_count() > 1;

        for ( const auto& point: shape->shape.get() )
        {
            Object pobj;
            auto pto = point.polar_tan_out();
            auto pti = point.polar_tan_in();

            if ( animated || (
                point.type == math::bezier::PointType::Corner && (!qFuzzyIsNull(pto.length) || !qFuzzyIsNull(pti.length))
            ) )
            {
                pobj = types.object(TypeId::CubicDetachedVertex);
                pobj.set("outRotation"_qs, pto.angle);
                pobj.set("outDistance"_qs, pto.length);
                pobj.set("inRotation"_qs, pti.angle);
                pobj.set("inDistance"_qs, pti.length);
            }
            else if ( point.type == math::bezier::PointType::Symmetrical)
            {
                pobj = types.object(TypeId::CubicMirroredVertex);
                pobj.set("rotation"_qs, pto.angle);
                pobj.set("distance"_qs, pto.length);
            }
            else if ( point.type == math::bezier::PointType::Smooth )
            {
                pobj = types.object(TypeId::CubicAsymmetricVertex);
                pobj.set("rotation"_qs, pto.angle);
                pobj.set("outDistance"_qs, pto.length);
                pobj.set("inDistance"_qs, pti.length);
            }
            else
            {
                pobj = types.object(TypeId::StraightVertex);
            }

            pobj.set("parentId"_qs, id);
            pobj.set("x"_qs, point.pos.x());
            pobj.set("y"_qs, point.pos.y());
            serializer.write_object(pobj);
            next_artboard_child++;
        }

        if ( animated )
        {
            auto type = types.get_type(TypeId::CubicDetachedVertex);
            const Identifier prop_x = 0;
            const Identifier prop_y = 1;
            const Identifier prop_in_rot = 2;
            const Identifier prop_in_len = 3;
            const Identifier prop_out_rot = 4;
            const Identifier prop_out_len = 5;

            auto kf_type = types.get_type(TypeId::KeyFrameDouble);
            std::array<std::pair<Identifier, std::vector<Object>>, 6> props_template{{
                {type->property("x"_qs)->id, {}},
                {type->property("y"_qs)->id, {}},
                {type->property("inRotation"_qs)->id, {}},
                {type->property("inDistance"_qs)->id, {}},
                {type->property("outRotation"_qs)->id, {}},
                {type->property("outDistance"_qs)->id, {}}
            }};

            int point_count = next_artboard_child - first_point_id;
            for ( auto pt_id = 0; pt_id < point_count; pt_id++ )
            {
                auto props = props_template;

                for ( const auto& kf : shape->shape )
                {
                    if ( int(pt_id) >= kf.get().size() )
                    {
                        format->error(QObject::tr("Bezier has mismatching number of points"));
                        continue;
                    }

                    for ( auto& prop: props )
                    {
                        Object rive_kf(kf_type);
                        /// \todo interpolations
                        rive_kf.set("interpolationType"_qs, 1);
                        rive_kf.set("frame"_qs, kf.time());
                        prop.second.push_back(std::move(rive_kf));
                    }

                    auto point = kf.get()[pt_id];
                    auto pto = point.polar_tan_out();
                    auto pti = point.polar_tan_in();
                    props[prop_x].second.back().set("value"_qs, point.pos.x());
                    props[prop_y].second.back().set("value"_qs, point.pos.y());
                    props[prop_in_rot].second.back().set("value"_qs, pti.angle);
                    props[prop_in_len].second.back().set("value"_qs, pti.length);
                    props[prop_out_rot].second.back().set("value"_qs, pto.angle + math::pi);
                    props[prop_out_len].second.back().set("value"_qs, pto.length);
                }

                auto& keyed_point = animations[first_point_id + pt_id];
                for ( const auto& prop : props )
                {
                    keyed_point.emplace_back(types.get_type(TypeId::KeyedProperty));
                    keyed_point.back().set("propertyKey"_qs, prop.first);
                    for ( auto& rkf : prop.second )
                        keyed_point.emplace_back(std::move(rkf));
                }
            }
        }
    }

    Object shape_object(TypeId type_id, model::DocumentNode* shape, Identifier parent_id)
    {
        auto object = types.object(type_id);
        object.set("name"_qs, shape->name.get());
        object.set("parentId"_qs, parent_id);
        return object;
    }

    void write_group(Object& object, model::Group* group, Identifier id)
    {
        write_property(object, "opacity"_qs, group->opacity, id, &detail::noop);
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

        object.set(rive_prop, transform(prop.value(), 0));

        if ( !prop.animated() )
            return;

        const ObjectType* kf_type = nullptr;
        QString attr;
        switch ( rive_prop->type )
        {
            case PropertyType::Float:
            case PropertyType::VarUint:
                attr = "value"_qs;
                kf_type = types.get_type(TypeId::KeyFrameDouble);
                break;
            case PropertyType::Color:
                attr = "colorValue"_qs;
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
        keyed_prop.set("propertyKey"_qs, rive_prop->id);
        keyed_object.emplace_back(std::move(keyed_prop));

        for ( const auto& kf : prop )
        {
            Object rive_kf(kf_type);
            /// \todo interpolations
            rive_kf.set("interpolationType"_qs, 1);
            rive_kf.set(attr, transform(kf.value(), kf.time()));
            rive_kf.set("frame"_qs, kf.time());
            keyed_object.emplace_back(std::move(rive_kf));
        }
    }

    void write_position(Object& object, const model::AnimatedProperty<QPointF>& prop, Identifier object_id)
    {
        write_property(object, "x"_qs, prop, object_id,
            [](const QVariant& v, model::FrameTime) { return QVariant::fromValue(v.toPointF().x()); }
        );
        write_property(object, "y"_qs, prop, object_id,
            [](const QVariant& v, model::FrameTime) { return QVariant::fromValue(v.toPointF().y()); }
        );
    }

    void write_transform(Object& object, model::Transform* trans, Identifier object_id, const QRectF& box)
    {
        if ( object.type().property("originX"_qs) )
        {
            write_position(object, trans->position, object_id);

            if ( box.width() > 0 )
            {
                write_property(object, "originX"_qs, trans->anchor_point, object_id,
                    [&box](const QVariant& v, model::FrameTime) {
                        return QVariant::fromValue(
                            (v.toPointF().x() - box.left()) / box.width()
                        );
                    }
                );
            }

            if ( box.height() > 0 )
            {
                write_property(object, "originY"_qs, trans->anchor_point, object_id,
                    [&box](const QVariant& v, model::FrameTime) {
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
            write_property(object, "x"_qs, trans->position, object_id,
                [anchor](const QVariant& v, model::FrameTime) { return QVariant::fromValue(v.toPointF().x() - anchor.x()); }
            );
            write_property(object, "y"_qs, trans->position, object_id,
                [anchor](const QVariant& v, model::FrameTime) { return QVariant::fromValue(v.toPointF().y() - anchor.y()); }
            );
        }

        write_property(object, "rotation"_qs, trans->rotation, object_id, &detail::noop);

        write_property(object, "scaleX"_qs, trans->scale, object_id,
            [](const QVariant& v, model::FrameTime) { return QVariant::fromValue(v.value<QVector2D>().x()); }
        );
        write_property(object, "scaleY"_qs, trans->scale, object_id,
            [](const QVariant& v, model::FrameTime) { return QVariant::fromValue(v.value<QVector2D>().x()); }
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
            write_property(object, "opacity"_qs, shape->color, id, &detail::noop);

            serializer.write_object(object);
            /// \todo finish
        }
        else if ( auto col = use->cast<model::NamedColor>() )
        {
            auto object = shape_object(TypeId::SolidColor, col, object_id);
            write_property(object, "colorValue"_qs, col->color, id, &detail::noop);
            serializer.write_object(object);
        }
        else
        {
            auto object = shape_object(TypeId::SolidColor, shape, object_id);
            write_property(object, "colorValue"_qs, shape->color, id, &detail::noop);
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
