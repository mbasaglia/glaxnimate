#pragma once


#include <set>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "lottie_private_common.hpp"

namespace io::lottie::detail {

struct FontInfo
{
    QString name;
    QString family;
    QString style;
};

class LottieImporterState
{
public:
    LottieImporterState(
        model::Document* document,
        io::lottie::LottieFormat* format
    ) : document(document), format(format)
    {}

    void load(const QJsonObject& json)
    {
        load_animation_container(json, document->main()->animation.get());
        load_assets(json["assets"].toArray());
        load_fonts(json["fonts"]["list"].toArray());
        load_composition(json, document->main());
    }

private:
    void warning(QString str, const QJsonObject& json)
    {
        if ( json.contains("nm") )
            str = json["nm"].toString() + ": " + str;
        emit format->warning(str);
    }

    void load_stretchable_animation_container(const QJsonObject& json, model::StretchableTime* animation)
    {
        animation->start_time.set(json["st"].toDouble());
        animation->stretch.set(json["sr"].toDouble(1));
    }

    void load_animation_container(const QJsonObject& json, model::AnimationContainer* animation)
    {
        animation->first_frame.set(json["ip"].toDouble());
        animation->last_frame.set(json["op"].toDouble());
    }

    void load_composition(const QJsonObject& json, model::Composition* composition)
    {
        this->composition = composition;
        invalid_indices.clear();
        layer_indices.clear();
        deferred.clear();

        load_basic(json, composition);

        {
            std::set<int> referenced;
            std::vector<QJsonObject> layer_jsons;
            auto layer_array = json["layers"].toArray();
            layer_jsons.reserve(layer_array.size());
            for ( auto val : layer_array )
            {
                QJsonObject obj = val.toObject();
                if ( obj.contains("parent") )
                    referenced.insert(obj["parent"].toInt());
                layer_array.push_back(obj);
            }

            for ( auto layer : json["layers"].toArray() )
                create_layer(layer.toObject(), referenced);
        }

        auto deferred_layers = std::move(deferred);
        deferred.clear();
        for ( const auto& pair: deferred_layers )
            load_layer(pair.second, static_cast<model::Layer*>(pair.first));
    }

    bool has_mask(const QJsonObject& json)
    {
        return mask && json["tt"].toInt();
    }

    void load_visibility(model::VisualNode* node, const QJsonObject& json)
    {
        if ( json.contains("hd") && json["hd"].toBool() )
            node->visible.set(false);
    }

    void create_layer(const QJsonObject& json, std::set<int>& referenced)
    {
        int index = json["ind"].toInt();
        if ( !json.contains("ty") || !json["ty"].isDouble() )
        {
            warning(QObject::tr("Missing layer type for %1").arg(index), json);
            invalid_indices.insert(index);
            return;
        }

        int ty = json["ty"].toInt();
        if ( ty == 0 )
        {
            load_precomp_layer(json, referenced);
            mask = nullptr;
        }
        else
        {
            auto layer = std::make_unique<model::Layer>(document);
            if ( json["td"].toInt() )
            {
                mask = layer.get();
                layer->mask->mask.set(true);
                layer->name.set(json["nm"].toString());
                auto child = std::make_unique<model::Layer>(document);
                layer_indices[index] = layer.get();
                deferred.emplace_back(child.get(), json);
                layer->shapes.insert(std::move(child), 0);
                composition->shapes.insert(std::move(layer), 0);
            }
            else
            {
                layer_indices[index] = layer.get();
                deferred.emplace_back(layer.get(), json);
                if ( has_mask(json) )
                    mask->shapes.insert(std::move(layer), 1);
                else
                    composition->shapes.insert(std::move(layer), 0);
                mask = nullptr;
            }
        }
    }

    void load_precomp_layer(const QJsonObject& json, std::set<int>& referenced)
    {
        auto props = load_basic_setup(json);

        auto precomp = std::make_unique<model::PreCompLayer>(document);
        load_visibility(precomp.get(), json);

        load_stretchable_animation_container(json, precomp->timing.get());

        for ( const FieldInfo& field : fields["__Layer__"] )
            props.erase(field.lottie);

        for ( const QMetaObject* mo = precomp->metaObject(); mo; mo = mo->superClass() )
            load_properties(
                precomp.get(),
                fields[model::detail::naked_type_name(mo)],
                json,
                props
            );

        auto comp = precomp_ids[json["refId"].toString()];
        if ( comp )
        {
            precomp->composition.set(comp);
            if ( !json.contains("nm") )
                precomp->name.set(comp->name.get());
        }
        props.erase("w");
        props.erase("h");
        precomp->size.set(QSize(
            json["w"].toInt(),
            json["h"].toInt()
        ));

        int index = json["ind"].toInt();
        auto op = document->main()->animation->last_frame.get();
        if ( json.contains("parent") || referenced.count(index) || json["ip"].toDouble() != 0 ||
            json["op"].toDouble(op) != op || has_mask(json)
        )
        {
            auto layer = std::make_unique<model::Layer>(document);
            layer->name.set(precomp->name.get());
            layer->shapes.insert(std::move(precomp), 0);
            if ( has_mask(json) )
                layer->mask->mask.set(mask);
            layer_indices[index] = layer.get();
            deferred.emplace_back(layer.get(), json);
            load_transform(json["ks"].toObject(), layer->transform.get(), &layer->opacity);
            composition->shapes.insert(std::move(layer), 0);
        }
        else
        {
            load_transform(json["ks"].toObject(), precomp->transform.get(), &precomp->opacity);
            composition->shapes.insert(std::move(precomp), 0);
        }
    }

    void load_mask(const QJsonObject& json, model::Group* group)
    {
        auto fill = std::make_unique<model::Fill>(document);
        fill->color.set(QColor(255, 255, 255));
        load_animated(&fill->opacity, json["o"], {});
        document->set_best_name(fill.get());
        group->shapes.insert(std::move(fill));

        auto j_stroke = json["x"].toObject();
        if ( j_stroke["a"].toInt() || j_stroke["k"].toDouble() != 0 )
        {
            auto stroke = std::make_unique<model::Stroke>(document);
            stroke->color.set(QColor(255, 255, 255));
            load_animated(&stroke->opacity, json["o"], {});
            load_animated(&stroke->width, json["x"], {});
            document->set_best_name(stroke.get());
            group->shapes.insert(std::move(stroke));
        }

        auto path = std::make_unique<model::Path>(document);
        load_animated(&path->shape, json["pt"], {});
        document->set_best_name(path.get());
        group->shapes.insert(std::move(path));
    }

    void load_layer(const QJsonObject& json, model::Layer* layer)
    {
        if ( json.contains("parent") )
        {
            int parent_index = json["parent"].toInt();
            if ( invalid_indices.count(parent_index) )
            {
                warning(
                    QObject::tr("Cannot use %1 as parent as it couldn't be loaded")
                    .arg(parent_index),
                    json
                );
            }
            else
            {
                auto it = layer_indices.find(parent_index);
                if ( it == layer_indices.end() )
                {
                    warning(
                        QObject::tr("Invalid parent layer %1")
                        .arg(parent_index),
                        json
                    );
                }
                else
                {
                    auto parent_layer = layer->docnode_parent()->cast<model::Layer>();
                    if ( parent_layer && parent_layer->mask->has_mask() )
                        parent_layer->parent.set(*it);
                    else
                        layer->parent.set(*it);
                }
            }
        }

        load_animation_container(json, layer->animation.get());

        if ( !layer->shapes.empty() )
            return;

        auto props = load_basic_setup(json);
        props.erase("ind");

        load_properties(layer, fields["DocumentNode"], json, props);
        load_properties(layer, fields["__Layer__"], json, props);

        load_transform(json["ks"].toObject(), layer->transform.get(), &layer->opacity);
        load_visibility(layer, json);

        model::Layer* target = layer;
        props.erase("hasMask");
        props.erase("masksProperties");
        if ( json.contains("masksProperties") )
        {
            auto masks = json["masksProperties"].toArray();
            if ( !masks.empty() )
            {
                layer->mask->mask.set(true);

                auto clip_p = std::make_unique<model::Group>(document);
                auto clip = clip_p.get();
                layer->shapes.insert(std::move(clip_p));
                auto shape_target = std::make_unique<model::Layer>(document);
                target = shape_target.get();
                shape_target->name.set(layer->name.get());
                layer->shapes.insert(std::move(shape_target));

                document->set_best_name(clip, QObject::tr("Clip"));
                if ( masks.size() == 1 )
                {
                    load_mask(masks[0].toObject(), clip);
                }
                else
                {
                    for ( const auto& mask : masks )
                    {
                        auto clip_group_p = std::make_unique<model::Group>(document);
                        auto clip_group = clip_group_p.get();
                        clip->shapes.insert(std::move(clip_group_p));
                        document->set_best_name(clip_group, QObject::tr("Clip"));
                        load_mask(mask.toObject(), clip_group);
                    }
                }
            }
        }

        switch ( json["ty"].toInt(-1) )
        {
            case 0: // precomp
                break;
            case 1: // solid color
            {
                props.erase("sw");
                props.erase("sh");
                props.erase("sc");
                auto rect = std::make_unique<model::Rect>(document);
                rect->size.set(QSizeF(
                    json["sw"].toDouble(),
                    json["sh"].toDouble()
                ));
                target->shapes.insert(std::move(rect));

                auto fill = std::make_unique<model::Fill>(document);
                fill->color.set(QColor(json["sc"].toString()));
                target->shapes.insert(std::move(fill));
                break;
            }
            case 2: // image layer
            {
                auto image = std::make_unique<model::Image>(document);
                image->image.set(bitmap_ids[json["refId"].toString()]);
                target->shapes.insert(std::move(image));
                props.erase("refId");
                break;
            }
            case 3: // empty
                break;
            case 4: // shape
                props.erase("shapes");
                load_shapes(target->shapes, json["shapes"].toArray());
                break;
            case 5: // text
                props.erase("t");
                load_text_layer(target->shapes, json["t"].toObject());
                break;
            default:
            {
                QString type = json["ty"].toVariant().toString();
                auto it = unsupported_layers.find(json["ty"].toInt());
                if ( it != unsupported_layers.end() )
                    type = *it;
                warning(QObject::tr("Unsupported layer of type %1").arg(type), json);
            }
        }

        load_basic_check(props);
    }

    void load_shapes(model::ShapeListProperty& shapes, const QJsonArray& jshapes)
    {
        deferred.clear();

        for ( int i = jshapes.size() - 1; i >= 0; i-- )
            create_shape(jshapes[i].toObject(), shapes);

        auto deferred_shapes = std::move(deferred);
        deferred.clear();

        for ( const auto& pair: deferred_shapes )
            load_shape(pair.second, static_cast<model::ShapeElement*>(pair.first));
    }

    void create_shape(const QJsonObject& json, model::ShapeListProperty& shapes)
    {
        if ( !json.contains("ty") || !json["ty"].isString() )
        {
            warning(QObject::tr("Missing shape type"), json);
            return;
        }

        QString base_type = json["ty"].toString();
        QString type = shape_types.key(base_type);
        if ( type.isEmpty() )
        {
            type = shape_types_repeat[base_type];
            if ( type.isEmpty() )
            {
                // "mm" is marked as unsupported by lottie and it appears in several animations so we ignore the warning
                if ( base_type != "mm" )
                    warning(QObject::tr("Unsupported shape type %1").arg(json["ty"].toString()), json);
                return;
            }
        }

        model::ShapeElement* shape = static_cast<model::ShapeElement*>(
            model::Factory::instance().build(type, document)
        );
        if ( !shape )
        {
            warning(QObject::tr("Unsupported shape type %1").arg(json["ty"].toString()), json);
            return;
        }

        deferred.emplace_back(shape, json);
        shapes.insert(std::unique_ptr<model::ShapeElement>(shape), shapes.size());
    }

    std::set<QString> load_basic_setup(const QJsonObject& json_obj)
    {
        std::set<QString> props;

        for ( auto it = json_obj.begin(); it != json_obj.end(); ++it )
            props.insert(it.key());

        return props;
    }

    void load_basic_check(const std::set<QString>& props)
    {
        for ( const auto& not_found : props )
            emit format->information(QObject::tr("Unknown field %1").arg(not_found));
    }

    void load_basic(const QJsonObject& json_obj, model::Object* obj)
    {
        std::set<QString> props = load_basic_setup(json_obj);

        for ( const QMetaObject* mo = obj->metaObject(); mo; mo = mo->superClass() )
            load_properties(
                obj,
                fields[model::detail::naked_type_name(mo)],
                json_obj,
                props
            );

        load_basic_check(props);
    }

    void load_basic(const QJsonObject& json_obj, model::DocumentNode* obj)
    {
        load_basic(json_obj, static_cast<model::Object*>(obj));
        if ( obj->name.get().isEmpty() )
            document->set_best_name(obj);
    }

    void load_transform(const QJsonObject& transform, model::Transform* tf, model::AnimatableBase* opacity)
    {
        load_basic(transform, tf);
        if ( transform.contains("o") )
            load_animated(opacity, transform["o"], FloatMult(100));
    }

    void load_styler(model::Styler* styler, const QJsonObject& json_obj)
    {
        std::set<QString> props = load_basic_setup(json_obj);
        for ( const QMetaObject* mo = styler->metaObject(); mo; mo = mo->superClass() )
            load_properties(
                styler,
                fields[model::detail::naked_type_name(mo)],
                json_obj,
                props
            );

        if ( json_obj["ty"].toString().startsWith('g') )
        {
            auto gradient = document->assets()->gradients->values.insert(std::make_unique<model::Gradient>(document));
            styler->use.set(gradient);
            auto colors = document->assets()->gradient_colors->values.insert(std::make_unique<model::GradientColors>(document));
            gradient->colors.set(colors);
            load_properties(gradient, fields["Gradient"], json_obj, props);

            /// \todo load highlight from h/a if present
            gradient->highlight.set(gradient->start_point.get());

            auto jcolors = json_obj["g"].toObject();
            load_animated(&colors->colors, jcolors["k"], GradientLoad{jcolors["p"].toInt()});
        }

        if ( styler->name.get().isEmpty() )
            document->set_best_name(styler);

        load_basic_check(props);
    }

    void load_shape(const QJsonObject& json, model::ShapeElement* shape)
    {
        if ( auto styler = shape->cast<model::Styler>() )
            return load_styler(styler, json);

        load_basic(json, shape);
        load_visibility(shape, json);

        if ( shape->type_name() == "Group" )
        {
            auto gr = static_cast<model::Group*>(shape);
            QJsonArray shapes = json["it"].toArray();
            QJsonObject transform;

            for ( int i = shapes.size() - 1; i >= 0; i-- )
            {
                QJsonObject shi = shapes[i].toObject();
                if ( shi["ty"] == "tr" )
                {
                    transform = shi;
                    transform.remove("ty");
                    shapes.erase(shapes.begin() + i);
                    break;
                }
            }
            if ( !transform.empty() )
                load_transform(transform, gr->transform.get(), &gr->opacity);

            load_shapes(gr->shapes, shapes);
        }
    }

    void load_properties(
        model::Object* obj,
        const QVector<FieldInfo>& fields,
        const QJsonObject& json_obj,
        std::set<QString>& avail_obj_keys
    )
    {
        for ( const FieldInfo& field : fields )
        {
            avail_obj_keys.erase(field.lottie);
            if ( field.mode != Auto || !json_obj.contains(field.lottie) )
                continue;

            model::BaseProperty * prop = obj->get_property(field.name);
            if ( !prop )
            {
                logger.stream() << field.name << "is not a property";
                continue;
            }

            if ( prop->traits().flags & model::PropertyTraits::Animated )
            {
                load_animated(static_cast<model::AnimatableBase*>(prop), json_obj[field.lottie], field.transform);
            }
            else
            {
                load_value(prop, json_obj[field.lottie], field.transform);
            }
        }
    }

    template<class T>
    bool compound_value_2d_raw(const QJsonValue& val, T& out, double mul = 1)
    {
        QJsonArray arr = val.toArray();
        if ( arr.size() < 2 || !arr[0].isDouble() || !arr[1].isDouble() )
            return false;

        out = T(arr[0].toDouble() * mul, arr[1].toDouble() * mul);
        return true;
    }

    template<class T>
    std::optional<QVariant> compound_value_2d(const QJsonValue& val, double mul = 1)
    {
        T v;
        if ( !compound_value_2d_raw(val, v, mul) )
            return {};
        return QVariant::fromValue(v);
    }

    bool is_scalar(model::BaseProperty * prop)
    {
        switch ( prop->traits().type )
        {
            case model::PropertyTraits::Bool:
            case model::PropertyTraits::Int:
            case model::PropertyTraits::Float:
            case model::PropertyTraits::String:
            case model::PropertyTraits::Uuid:
            case model::PropertyTraits::Enum:
            case model::PropertyTraits::Bezier:
                return true;
            default:
                return false;
        }
    }

    bool compound_value_color(const QJsonValue& val, QColor& out)
    {
        QJsonArray arr = val.toArray();
        if ( arr.size() == 3 )
            out = QColor::fromRgbF(
                arr[0].toDouble(), arr[1].toDouble(), arr[2].toDouble()
            );
        else if ( arr.size() == 4 )
            out = QColor::fromRgbF(
                arr[0].toDouble(), arr[1].toDouble(), arr[2].toDouble(), arr[3].toDouble()
            );
        else
            return false;

        return true;
    }

    std::optional<QVariant> value_to_variant(model::BaseProperty * prop, const QJsonValue& val)
    {
        switch ( prop->traits().type )
        {
            case model::PropertyTraits::Bool:
            case model::PropertyTraits::Int:
            case model::PropertyTraits::Float:
            case model::PropertyTraits::String:
            case model::PropertyTraits::Uuid:
                return val.toVariant();
            case model::PropertyTraits::Point:
                return compound_value_2d<QPointF>(val);
            case model::PropertyTraits::Size:
                return compound_value_2d<QSizeF>(val);
            case model::PropertyTraits::Scale:
                return compound_value_2d<QVector2D>(val, 0.01);
            case model::PropertyTraits::Color:
            {
                QColor col;
                if ( compound_value_color(val, col) )
                    return QVariant::fromValue(col);
                return {};
            }
            case model::PropertyTraits::Bezier:
            {
                QJsonObject jsbez = val.toObject();
                math::bezier::Bezier bezier;
                bezier.set_closed(jsbez["c"].toBool());
                QJsonArray pos = jsbez["v"].toArray();
                QJsonArray tan_in = jsbez["i"].toArray();
                QJsonArray tan_out = jsbez["o"].toArray();
                int sz = std::min(pos.size(), std::min(tan_in.size(), tan_out.size()));
                for ( int i = 0; i < sz; i++ )
                {
                    QPointF p, ti, to;
                    if ( !compound_value_2d_raw(pos[i], p) )
                    {
                        emit format->warning(
                            QObject::tr("Invalid bezier point %1")
                            .arg(i)
                        );
                        continue;
                    }
                    compound_value_2d_raw(tan_in[i], ti);
                    compound_value_2d_raw(tan_out[i], to);
                    bezier.push_back(math::bezier::Point::from_relative(p, ti, to));
                }
                return QVariant::fromValue(bezier);
            }
            case model::PropertyTraits::Enum:
                return val.toInt();
            case model::PropertyTraits::Gradient:
                return val.toArray().toVariantList();
            default:
                logger.stream(app::log::Error) << "Unsupported type" << prop->traits().type << "for" << prop->name();
                return {};
        }
    }

    void load_value(model::BaseProperty * prop, const QJsonValue& val, const TransformFunc& trans)
    {
        auto v = value_to_variant(prop, val);
        if ( !v || !prop->set_value(trans.from_lottie(*v, 0)) )
            emit format->warning(QObject::tr("Invalid value for %1").arg(prop->name()));
    }

    void load_animated(model::AnimatableBase* prop, const QJsonValue& val, const TransformFunc& trans)
    {
        if ( !val.isObject() )
        {
            emit format->warning(QObject::tr("Invalid value for %1").arg(prop->name()));
            return;
        }

        QJsonObject obj = val.toObject();
        if ( !obj.contains("a") || !obj.contains("k") )
        {
            emit format->warning(QObject::tr("Invalid value for %1").arg(prop->name()));
            return;
        }

        if ( obj["a"].toInt() )
        {
            if ( !obj["k"].isArray() )
            {
                emit format->warning(QObject::tr("Invalid keyframes for %1").arg(prop->name()));
                return;
            }

            /// @todo for position fields also add spatial bezier handles
            for ( QJsonValue jkf : obj["k"].toArray() )
            {
                model::FrameTime time = jkf["t"].toDouble();
                QJsonValue s = jkf["s"];
                if ( s.isArray() && is_scalar(prop) )
                    s = s.toArray()[0];

                auto v = value_to_variant(prop, s);
                model::KeyframeBase* kf = nullptr;
                if ( v )
                    kf = prop->set_keyframe(time, trans.from_lottie(*v, time));

                if ( kf )
                {
                    kf->set_transition({
                        keyframe_bezier_handle(jkf["o"]),
                        keyframe_bezier_handle(jkf["i"]),
                        bool(jkf["h"].toInt())
                    });
                }
                else
                {
                    emit format->warning(QObject::tr("Cannot load keyframe at %1 for %2")
                        .arg(time).arg(prop->name())
                    );
                }
            }
        }
        else
        {
            load_value(prop, obj["k"], trans);
        }
    }

    QPointF keyframe_bezier_handle(const QJsonValue& val)
    {
        QJsonObject jobj = val.toObject();
        return {jobj["x"].toDouble(), jobj["y"].toDouble()};
    }

    void load_assets(const QJsonArray& assets)
    {
        std::vector<std::pair<QJsonObject, model::Precomposition*>> comps;

        for ( const auto& assetv : assets )
        {
            QJsonObject asset = assetv.toObject();
            if ( asset.contains("e") && asset.contains("p") && asset.contains("w") )
                load_asset_bitmap(asset);
            else if ( asset.contains("layers") )
                comps.emplace_back(asset, load_asset_precomp(asset));
        }

        for ( const auto& p : comps )
            load_composition(p.first, p.second);
    }

    void load_asset_bitmap(const QJsonObject& asset)
    {
        auto bmp = document->assets()->images->values.insert(std::make_unique<model::Bitmap>(document));

        QString id = asset["id"].toString();
        if ( bitmap_ids.count(id) )
            format->warning(io::lottie::LottieFormat::tr("Duplicate Bitmap ID: %1").arg(id));
        bitmap_ids[id] = bmp;

        if ( asset["e"].toInt() )
        {
            bmp->from_url(asset["p"].toString());
        }
        else
        {
            QDir dir(asset["u"].toString());
            bmp->from_file(dir.filePath(asset["p"].toString()));
        }
    }

    model::Precomposition* load_asset_precomp(QJsonObject asset)
    {
        auto comp = document->assets()->precompositions->values.insert(std::make_unique<model::Precomposition>(document));

        QString id = asset["id"].toString();
        if ( precomp_ids.count(id) )
            format->warning(io::lottie::LottieFormat::tr("Duplicate Precomposition ID: %1").arg(id));
        precomp_ids[id] = comp;

        comp->name.set(id);
        return comp;
    }

    void load_fonts(const QJsonArray& fonts_arr)
    {
        for ( const auto& fontv : fonts_arr )
        {
            QJsonObject font = fontv.toObject();
            FontInfo info;
            info.family = font["fFamily"].toString();
            info.name = font["fName"].toString();
            info.style = font["fStyle"].toString();
            fonts[info.name] = info;
        }
    }

    FontInfo get_font(const QString& name)
    {
        auto it = fonts.find(name);
        if ( it != fonts.end() )
            return *it;
        return {"", name, "Regular"};
    }

    void load_text_layer(model::ShapeListProperty& shapes, const QJsonObject& text)
    {
        // TODO "a" "m" "p"

        model::Group* prev = nullptr;
        model::KeyframeTransition jump({}, {}, true);

        for ( const auto& v : text["d"].toObject()["k"].toArray() )
        {
            auto keyframe = v.toObject();
            qreal time = keyframe["t"].toDouble();
            auto text_document = keyframe["s"].toObject();

            auto group = std::make_unique<model::Group>(document);
            if ( time > 0 )
                group->opacity.set_keyframe(0, 0)->set_transition(jump);
            group->opacity.set_keyframe(time, 1)->set_transition(jump);
            if ( prev )
                prev->opacity.set_keyframe(time, 0)->set_transition(jump);
            prev = group.get();

            auto fill = std::make_unique<model::Fill>(document);
            QColor color;
            compound_value_color(text_document["fc"], color);
            fill->color.set(color);
            group->shapes.insert(std::move(fill));

            auto shape = std::make_unique<model::TextShape>(document);
            auto font = get_font(text_document["f"].toString());
            shape->font->family.set(font.family);
            shape->font->style.set(font.style);
            shape->font->size.set(text_document["s"].toDouble());
            shape->text.set(text_document["t"].toString().replace('\r', '\n'));
            group->shapes.insert(std::move(shape));

            shapes.insert(std::move(group), shapes.size());
        }
    }

    model::Document* document;
    io::lottie::LottieFormat* format;
    QMap<int, model::Layer*> layer_indices;
    std::set<int> invalid_indices;
    std::vector<std::pair<model::Object*, QJsonObject>> deferred;
    model::Composition* composition = nullptr;
    app::log::Log logger{"Lottie Import"};
    QMap<QString, model::Bitmap*> bitmap_ids;
    QMap<QString, model::Precomposition*> precomp_ids;
    QMap<QString, FontInfo> fonts;
    model::Layer* mask = nullptr;
};


} // namespace io::lottie::detail
