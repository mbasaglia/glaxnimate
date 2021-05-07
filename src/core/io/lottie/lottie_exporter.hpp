#pragma once


#include <QCborValue>
#include <QCborArray>

#include "cbor_write_json.hpp"
#include "lottie_private_common.hpp"

namespace io::lottie::detail {

QLatin1String operator "" _l(const char* c, std::size_t sz)
{
    return QLatin1String(c, sz);
}

class LottieExporterState
{
public:
    explicit LottieExporterState(io::lottie::LottieFormat* format, model::Document* document, bool strip)
        : format(format), document(document), strip(strip) {}

    QCborMap to_json()
    {
        return convert_main(document->main());
    }

    void convert_animation_container(model::AnimationContainer* animation, QCborMap& json)
    {
        json["ip"_l] = animation->first_frame.get();
        json["op"_l] = animation->last_frame.get();
    }

    void convert_composition(model::Composition* composition, QCborMap& json)
    {
        QCborArray layers;
        for ( const auto& layer : composition->shapes )
            if ( layer->visible.get() )
                convert_layer(layer_type(layer.get()), layer.get(), layers);

        json["layers"_l] = layers;
    }

    QCborMap convert_main(model::MainComposition* animation)
    {
        layer_indices.clear();
        QCborMap json;
        json["v"_l] = "5.5.2";
        convert_animation_container(animation->animation.get(), json);
        convert_object_basic(animation, json);
        json["assets"_l] = convert_assets();
        convert_composition(animation, json);
        return json;
    }

    int layer_index(model::DocumentNode* layer)
    {
        if ( !layer )
            return -1;
        if ( !layer_indices.contains(layer->uuid.get()) )
            layer_indices[layer->uuid.get()] = layer_indices.size();
        return layer_indices[layer->uuid.get()];
    }

    QCborMap wrap_layer_shape(model::ShapeElement* shape, model::Layer* forced_parent)
    {
        QCborMap json;
        json["ddd"_l] = 0;
        json["ty"_l] = 4;
        convert_fake_layer_parent(forced_parent, json);
        json["ind"_l] = layer_index(shape);
        json["st"_l] = 0;

        QCborMap transform;
        if ( auto grp = shape->cast<model::Group>() )
        {
            convert_transform(grp->transform.get(), &grp->opacity, transform);
        }
        else
        {
            model::Transform tf(document);
            convert_transform(&tf, nullptr, transform);
        }
        json["ks"_l] = transform;

        QCborArray shapes;
        shapes.push_back(convert_shape(shape));
        json["shapes"_l] = shapes;
        return json;
    }

    enum class LayerType { Shape, Layer, Image, PreComp };

    LayerType layer_type(model::ShapeElement* shape)
    {
        auto meta = shape->metaObject();
        if ( meta->inherits(&model::Layer::staticMetaObject) )
            return LayerType::Layer;
        if ( meta->inherits(&model::Image::staticMetaObject) )
            return LayerType::Image;
        if ( meta->inherits(&model::PreCompLayer::staticMetaObject) )
            return LayerType::PreComp;
        return LayerType::Shape;
    }

    QCborMap convert_single_layer(LayerType type, model::ShapeElement* shape, QCborArray& output, model::Layer* forced_parent, bool force_all_shapes)
    {
        switch ( type )
        {
            case LayerType::Shape:
                return wrap_layer_shape(shape, forced_parent);
            case LayerType::Image:
                return convert_image_layer(static_cast<model::Image*>(shape), forced_parent);
            case LayerType::PreComp:
                return convert_precomp_layer(static_cast<model::PreCompLayer*>(shape), forced_parent);
            case LayerType::Layer:
                break;
        }

        auto layer = static_cast<model::Layer*>(shape);

        int parent_index = layer_index(forced_parent ? forced_parent : layer->parent.get());

        QCborMap json;
        json["ddd"_l] = 0;
        json["ty"_l] = 3;
        int index = layer_index(layer);
        json["ind"_l] = index;
        json["st"_l] = 0;

        convert_animation_container(layer->animation.get(), json);
        convert_object_properties(layer, fields["DocumentNode"], json);
        convert_object_properties(layer, fields["__Layer__"], json);

        QCborMap transform;
        convert_transform(layer->transform.get(), &layer->opacity, transform);
        json["ks"_l] = transform;
        if ( parent_index != -1 )
            json["parent"_l] = parent_index;

        if ( !layer->shapes.empty() )
        {
            std::vector<LayerType> children_types;
            children_types.reserve(layer->shapes.size());

            bool all_shapes = true;
            if ( !force_all_shapes )
            {
                for ( const auto& shape : layer->shapes )
                {
                    children_types.push_back(layer_type(shape.get()));
                    if ( children_types.back() != LayerType::Shape )
                        all_shapes = false;
                }
            }

            if ( all_shapes )
            {
                json["ty"_l] = 4;
                json["shapes"_l] = convert_shapes(layer->shapes);
            }
            else
            {
                int i = 0;
                QCborMap mask;
                if ( layer->mask->has_mask() && !layer->shapes.empty() )
                {
                    if ( layer->shapes[0]->visible.get() )
                    {
                        mask = convert_single_layer(children_types[0], layer->shapes[0], output, layer, true);
                        if ( !mask.isEmpty() )
                            mask["td"_l] = 1;
                    }
                    i = 1;
                }

                for ( ; i < layer->shapes.size(); i++ )
                {
                    if ( layer->shapes[i]->visible.get() )
                        convert_layer(children_types[i], layer->shapes[i], output, layer, mask);
                }
            }
        }

        return json;
    }

    QCborMap convert_layer(LayerType type, model::ShapeElement* shape, QCborArray& output,
                           model::Layer* forced_parent = nullptr, const QCborMap& mask = {})
    {
        if ( !shape->visible.get() )
            return {};

        model::Layer* layer = nullptr;
        if ( type == LayerType::Layer )
        {
            layer = static_cast<model::Layer*>(shape);

            if ( !layer->render.get() )
                return {};
        }

        auto json = convert_single_layer(type, shape, output, forced_parent, false);

        if ( !mask.isEmpty() )
        {
            json["tt"_l] = 1;
            output.push_front(json);
            output.push_front(mask);
        }
        else
        {
            output.push_front(json);
        }

        return json;
    }

    void convert_transform(model::Transform* tf, model::AnimatableBase* opacity, QCborMap& json)
    {
        convert_object_basic(tf, json);
        if ( opacity )
            json["o"_l] = convert_animated(opacity, FloatMult(100));
        else
            json["o"_l] = fake_animated(100);
    }

    QCborArray point_to_lottie(const QPointF& vv)
    {
        return QCborArray{vv.x(), vv.y()};
    }

    QCborValue value_from_variant(const QVariant& v)
    {
        switch ( v.userType() )
        {
            case QMetaType::QPointF:
                return point_to_lottie(v.toPointF());
            case QMetaType::QVector2D:
            {
                auto vv = v.value<QVector2D>() * 100;
                return QCborArray{vv.x(), vv.y()};
            }
            case QMetaType::QSizeF:
            {
                auto vv = v.toSizeF();
                return QCborArray{vv.width(), vv.height()};
            }
            case QMetaType::QColor:
            {
                auto vv = v.value<QColor>().toRgb();
                return QCborArray{vv.redF(), vv.greenF(), vv.blueF(), vv.alphaF()};
            }
            case QMetaType::QUuid:
                return v.toString();
        }

        if ( v.userType() == qMetaTypeId<math::bezier::Bezier>() )
        {
            math::bezier::Bezier bezier = v.value<math::bezier::Bezier>();
            QCborMap jsbez;
            jsbez["c"_l] = bezier.closed();
            QCborArray pos, tan_in, tan_out;
            for ( const auto& p : bezier )
            {
                pos.push_back(point_to_lottie(p.pos));
                tan_in.push_back(point_to_lottie(p.tan_in - p.pos));
                tan_out.push_back(point_to_lottie(p.tan_out - p.pos));
            }
            jsbez["v"_l] = pos;
            jsbez["i"_l] = tan_in;
            jsbez["o"_l] = tan_out;
            return jsbez;
        }
        else if ( v.userType() == qMetaTypeId<QGradientStops>() )
        {
            QCborArray weird_ass_representation;
            auto gradient = v.value<QGradientStops>();
            bool alpha = false;
            for ( const auto& stop : gradient )
            {
                weird_ass_representation.push_back(stop.first);
                weird_ass_representation.push_back(stop.second.redF());
                weird_ass_representation.push_back(stop.second.greenF());
                weird_ass_representation.push_back(stop.second.blueF());
                alpha = alpha || stop.second.alpha() != 0;
            }
            if ( alpha )
            {
                for ( const auto& stop : gradient )
                {
                    weird_ass_representation.push_back(stop.first);
                    weird_ass_representation.push_back(stop.second.alphaF());
                }
            }
            return weird_ass_representation;
        }
        else if ( v.userType() >= QMetaType::User && v.canConvert<int>() )
        {
            return v.toInt();
        }
        return QCborValue::fromVariant(v);
    }

    void convert_object_from_meta(model::Object* obj, const QMetaObject* mo, QCborMap& json_obj)
    {
        if ( auto super = mo->superClass() )
            convert_object_from_meta(obj, super, json_obj);

        auto it = fields.find(model::detail::naked_type_name(mo));
        if ( it != fields.end() )
            convert_object_properties(obj, *it, json_obj);
    }

    void convert_object_basic(model::Object* obj, QCborMap& json_obj)
    {
        convert_object_from_meta(obj, obj->metaObject(), json_obj);
    }

    void convert_object_properties(model::Object* obj, const QVector<FieldInfo>& fields, QCborMap& json_obj)
    {
        for ( const auto& field : fields )
        {
            if ( field.mode != Auto || (strip && !field.essential) )
                continue;

            model::BaseProperty * prop = obj->get_property(field.name);
            if ( !prop )
            {
                logger.stream() << field.name << "is not a property";
                continue;
            }

            if ( prop->traits().flags & model::PropertyTraits::Animated )
            {
                json_obj[field.lottie] = convert_animated(static_cast<model::AnimatableBase*>(prop), field.transform);
            }
            else
            {
                json_obj[field.lottie] = value_from_variant(field.transform.to_lottie(prop->value(), 0));
            }
        }
    }

    QCborValue keyframe_value_from_variant(const QVariant& v)
    {
        auto cb = value_from_variant(v);
        if ( cb.isArray() )
            return cb;

        return QCborArray{cb};
    }

    QCborMap convert_animated(
        model::AnimatableBase* prop,
        const TransformFunc& transform_values
    )
    {
        /// @todo for position fields also add spatial bezier handles
        QCborMap jobj;
        if ( prop->keyframe_count() > 1 )
        {
            jobj["a"_l] = 1;
            QCborArray keyframes;
            QCborMap jkf;
            for ( int i = 0, e = prop->keyframe_count(); i < e; i++ )
            {
                auto kf = prop->keyframe(i);
                QVariant v = transform_values.to_lottie(kf->value(), kf->time());
                QCborValue kf_value = keyframe_value_from_variant(v);
                if ( i != 0 )
                {
                    jkf["e"_l] = kf_value;
                    keyframes.push_back(jkf);
                }

                jkf.clear();
                jkf["t"_l] = kf->time();
                jkf["s"_l] = kf_value;

                if ( i != e - 1 )
                {
                    if ( kf->transition().hold() )
                    {
                        jkf["h"_l] =  1;
                    }
                    else
                    {
                        jkf["h"_l] =  0;
                        jkf["o"_l] = keyframe_bezier_handle(kf->transition().before());
                        jkf["i"_l] = keyframe_bezier_handle(kf->transition().after());
                    }
                }
            }
            keyframes.push_back(jkf);
            jobj["k"_l] = keyframes;
        }
        else
        {
            jobj["a"_l] = 0;
            QVariant v = transform_values.to_lottie(prop->value(), 0);
            jobj["k"_l] = value_from_variant(v);
        }
        return jobj;
    }

    QCborMap keyframe_bezier_handle(const QPointF& p)
    {
        QCborMap jobj;
        QCborArray x;
        x.push_back(p.x());
        QCborArray y;
        y.push_back(p.y());
        jobj["x"_l] = x;
        jobj["y"_l] = y;
        return jobj;
    }

    void convert_styler(model::Styler* shape, QCborMap& jsh)
    {
        auto used = shape->use.get();
        if ( !used )
            return ;

        if ( auto color = qobject_cast<model::NamedColor*>(used) )
        {
            jsh["c"_l] = convert_animated(&color->color, {});
            return;
        }

        auto gradient = qobject_cast<model::Gradient*>(used);
        if ( !gradient || !gradient->colors.get() )
            return;

        jsh.remove("c"_l);
        convert_object_basic(gradient, jsh);

        if ( shape->type_name() == "Fill" )
            jsh["ty"_l] = "gf";
        else
            jsh["ty"_l] = "gs";

        /// \todo highlight
        jsh["h"_l] = fake_animated(0);
        jsh["a"_l] = fake_animated(0);

        auto colors = gradient->colors.get();
        QCborMap jcolors;
        jcolors["p"_l] = colors->colors.get().size();
        jcolors["k"_l] = convert_animated(&colors->colors, {});
        jsh["g"_l] = jcolors;
    }

    QCborMap convert_shape(model::ShapeElement* shape)
    {
        if ( auto text = shape->cast<model::TextShape>() )
        {
            auto conv = text->to_path();
            return convert_shape(conv.get());
        }

        QCborMap jsh;
        jsh["ty"_l] = shape_types[shape->type_name()];
//         jsh["d"] = 0;
        convert_object_basic(shape, jsh);
        if ( auto gr = qobject_cast<model::Group*>(shape) )
        {
            if ( qobject_cast<model::Layer*>(gr) )
                format->information(io::lottie::LottieFormat::tr("Lottie only supports layers in the top level"));
            auto shapes = convert_shapes(gr->shapes);
            QCborMap transform;
            transform["ty"_l] = "tr";
            convert_transform(gr->transform.get(), &gr->opacity, transform);
            shapes.push_back(transform);
            jsh["it"_l] = shapes;
        }
        else if ( auto styler = shape->cast<model::Styler>() )
        {
            convert_styler(styler, jsh);
        }
        else if ( shape->type_name() == "PolyStar" )
        {
            QCborMap fake = fake_animated(0);
            jsh["os"_l] = fake;
            jsh["is"_l] = fake;
        }

        return jsh;
    }

    QCborMap fake_animated(const QCborValue& val)
    {
        QCborMap fake;
        fake["a"_l] = 0;
        fake["k"_l] = val;
        return fake;
    }

    QCborArray convert_shapes(const model::ShapeListProperty& shapes)
    {
        QCborArray jshapes;
        for ( const auto& shape : shapes )
        {
            if ( shape->is_instance<model::Image>() )
                format->warning(io::lottie::LottieFormat::tr("Images cannot be grouped with other shapes"));
            else if ( shape->is_instance<model::PreCompLayer>() )
                format->warning(io::lottie::LottieFormat::tr("Composition layers cannot be grouped with other shapes"));
            else
                jshapes.push_front(convert_shape(shape.get()));
        }
        return jshapes;
    }

    QCborArray convert_assets()
    {
        QCborArray assets;

        for ( const auto& bmp : document->assets()->images->values )
            assets.push_back(convert_bitmat(bmp.get()));


        for ( const auto& comp : document->assets()->precompositions->values )
            assets.push_back(convert_precomp(comp.get()));

        return assets;
    }

    QCborMap convert_bitmat(model::Bitmap* bmp)
    {
        QCborMap out;
        convert_object_basic(bmp, out);
        out["id"_l] = bmp->uuid.get().toString();
        out["e"_l] = int(bmp->embedded());
        if ( bmp->embedded() )
        {
            out["u"_l] = "";
            out["p"_l] = bmp->to_url().toString();
        }
        else
        {
            auto finfo = bmp->file_info();
            out["u"_l] = finfo.absolutePath();
            out["p"_l] = finfo.fileName();
        }
        return out;
    }

    void convert_fake_layer_parent(model::Layer* parent, QCborMap& json)
    {
        if ( parent )
        {
            convert_animation_container(parent->animation.get(), json);
            json["parent"_l] = layer_index(parent);
        }
        else
        {
            convert_animation_container(document->main()->animation.get(), json);
        }
    }

    void convert_fake_layer(model::DocumentNode* node, model::Layer* parent, QCborMap& json)
    {
        json["ddd"_l] = 0;
        if ( !strip )
        {
            json["nm"_l] = node->name.get();
            json["mn"_l] = node->uuid.get().toString();
        }
        convert_fake_layer_parent(parent, json);
        json["ind"_l] = layer_index(node);
    }

    QCborMap convert_image_layer(model::Image* image, model::Layer* parent)
    {
        QCborMap json;
        convert_fake_layer(image, parent, json);
        json["ty"_l] = 2;
        json["ind"_l] = layer_index(image);
        json["st"_l] = 0;
        QCborMap transform;
        convert_object_basic(image->transform.get(), transform);
        transform["o"_l] = QCborMap{
            {"a"_l, 0},
            {"k"_l, 100},
        };
        json["ks"_l] = transform;
        if ( image->image.get() )
            json["refId"_l] = image->image->uuid.get().toString();
        return json;
    }

    QCborMap convert_precomp(model::Precomposition* comp)
    {
        QCborMap out;
        convert_object_basic(comp, out);
        out["id"_l] = comp->uuid.get().toString();
        convert_composition(comp, out);
        return out;
    }

    QCborMap convert_precomp_layer(model::PreCompLayer* layer, model::Layer* parent)
    {
        QCborMap json;
        json["ty"_l] = 0;
        convert_fake_layer(layer, parent, json);
        json["ind"_l] = layer_index(layer);
        json["st"_l] = layer->timing->start_time.get();
        json["sr"_l] = layer->timing->stretch.get();
        QCborMap transform;
        convert_transform(layer->transform.get(), &layer->opacity, transform);
        json["ks"_l] = transform;
        if ( layer->composition.get() )
            json["refId"_l] = layer->composition->uuid.get().toString();
        json["w"_l] = layer->size.get().width();
        json["h"_l] = layer->size.get().height();
        return json;
    }

    io::lottie::LottieFormat* format;
    model::Document* document;
    bool strip;
    QMap<QUuid, int> layer_indices;
    app::log::Log logger{"Lottie Export"};
    model::Layer* mask = 0;
};



} // namespace io::lottie::detail
