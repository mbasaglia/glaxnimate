#include "lottie_format.hpp"

#include <set>

#include <QJsonObject>
#include <QJsonArray>

#include "app/log/log.hpp"
#include "model/layers/layers.hpp"
#include "model/shapes/shapes.hpp"

using namespace model;

io::Autoreg<io::lottie::LottieFormat> io::lottie::LottieFormat::autoreg;

namespace  {

enum FieldMode
{
    Ignored,
    Auto,
    Custom
};

struct FieldInfo
{
    QString name;
    QString lottie;
    bool essential;
    FieldMode mode;

    FieldInfo(const char* name, const char* lottie, bool essential = true)
        : name(name), lottie(lottie), essential(essential), mode(Auto)
    {}
    FieldInfo(const char* lottie, FieldMode mode = Ignored)
        : lottie(lottie), essential(false), mode(mode)
    {}
};

// static mapping data
const QMap<QString, QVector<FieldInfo>> fields = {
    {"DocumentNode", {
        FieldInfo{"name",           "nm", false},
        FieldInfo{"uuid",           "mn", false},
    }},
    {"AnimationContainer", {
        FieldInfo{"last_frame",     "op"},
        FieldInfo{"first_frame",     "ip"},
    }},
    {"Composition", {
        FieldInfo("layers", Custom),
    }},
    {"MainComposition", {
        FieldInfo("v", Custom),
        FieldInfo{"fps",            "fr"},
        FieldInfo{"width",          "w"},
        FieldInfo{"height",         "h"},
        FieldInfo("ddd"),
        FieldInfo("assets"),
        FieldInfo("comps"),
        FieldInfo("fonts"),
        FieldInfo("chars"),
        FieldInfo("markers"),
        FieldInfo("motion_blur"),
    }},
    {"Layer", {
        FieldInfo("ddd"),
        FieldInfo("hd"),
        FieldInfo("ty", Custom),
        FieldInfo("parent", Custom),
        FieldInfo("sr"),
        FieldInfo("ks", Custom),
        FieldInfo("ao"),
        FieldInfo{"start_time",     "st"},
        FieldInfo("bm"),
        FieldInfo("tt"),
        FieldInfo("ind", Custom),
        FieldInfo("cl"),
        FieldInfo("ln"),
        FieldInfo("hasMasks"),
        FieldInfo("masksProperties"),
        FieldInfo("ef"),
    }},
    {"SolidColorLayer", {
        FieldInfo{"sc", Custom},
        FieldInfo{"height",         "sh"},
        FieldInfo{"width",          "sw"},
    }},
    {"ShapeLayer", {
        FieldInfo{"shapes", Custom},
    }},
    {"Transform", {
        FieldInfo{"anchor_point",   "a"},
        FieldInfo("px", Custom),
        FieldInfo("py", Custom),
        FieldInfo("pz", Custom),
        FieldInfo{"position",       "p"},
        FieldInfo{"scale",          "s"},
        FieldInfo{"rotation",       "r"},
        FieldInfo("o"),
        FieldInfo("sk"),
        FieldInfo("sa"),
    }},
    {"ShapeElement", {
        FieldInfo{"ty", Custom},
        FieldInfo{"ix"},
        FieldInfo{"bm"},
        FieldInfo{"hd"},
    }},
    {"Shape", {
        FieldInfo{"d"},
    }},
    {"Rect", {
        FieldInfo{"position",       "p"},
        FieldInfo{"size",           "s"},
        FieldInfo{"rounded",        "r"},
    }},
    {"Ellipse", {
        FieldInfo{"position",       "p"},
        FieldInfo{"size",           "s"},
    }},
    {"Path", {
//         FieldInfo{"shape",          "ks"},
    }},
    {"Group", {
        FieldInfo{"np"},
        FieldInfo{"it", Custom},
    }},
    {"Fill", {
        FieldInfo{"o", Custom},
        FieldInfo{"color",          "c"},
        FieldInfo{"r", Custom},
    }},
    {"BaseStroke", {
        FieldInfo{"o", Custom},
        FieldInfo{"lc", Custom},
        FieldInfo{"lj", Custom},
        FieldInfo{"miter_limit",    "ml"},
        FieldInfo{"width",          "w"},
        FieldInfo{"d"},
    }},
    {"Stroke", {
        FieldInfo{"color",          "c"},
    }},
};
const QMap<QString, int> layer_types = {
    {"SolidColorLayer", 1},
    {"EmptyLayer", 3},
    {"ShapeLayer", 4}
};
const QMap<QString, QString> shape_types = {
    {"Rect", "rc"},
//     {"Star", "sr"},
    {"Ellipse", "el"},
    {"Path", "sh"},
    {"Group", "gr"},
    {"Fill", "fl"},
    {"Stroke", "st"},
//     {"GradientFill", "gf"},
//     {"GradientStroke", "gs"},
//     {"TransformShape", "tr"},
//     {"Trim", "tm"},
//     {"Repeater", "rp"},
//     {"EoundedCorners", "rd"},
//     {"Merge", "mm"},
//     {"Twist", "tw"},
};

class LottieExporterState
{
public:
    explicit LottieExporterState(model::Document* document)
        : document(document) {}

    QJsonObject to_json()
    {
        /// @todo make a system that preserves key order as that is needed for lottie android
        return convert_animation(document->main_composition());
    }

    QJsonObject convert_animation(MainComposition* animation)
    {
        layer_indices.clear();
        QJsonObject json = convert_object_basic(animation);
        json["v"] = "5.5.2";

        QJsonArray layers;
        for ( const auto& layer : animation->layers )
            layers.append(convert_layer(layer.get()));

        json["layers"] = layers;
        return json;
    }

    int layer_index(Layer* layer)
    {
        if ( !layer )
            return -1;
        if ( !layer_indices.contains(layer->uuid.get()) )
            layer_indices[layer->uuid.get()] = layer_indices.size();
        return layer_indices[layer->uuid.get()];
    }

    QJsonObject convert_layer(Layer* layer)
    {
        int parent_index = layer_index(layer->parent.get());
        QJsonObject json = convert_object_basic(layer);
        json["ty"] = layer_types[layer->type_name()];
        json["ind"] = layer_index(layer);

        QJsonObject transform = convert_transform(layer->transform.get(), &layer->opacity);
        json["ks"] = transform;
        if ( parent_index != -1 )
            json["parent"] = parent_index;

        if ( layer->type_name() == "SolidColorLayer" )
            json["sc"] = static_cast<SolidColorLayer*>(layer)->color.get().name();
        if ( layer->type_name() == "ShapeLayer" )
            json["shapes"] = convert_shapes(static_cast<ShapeLayer*>(layer)->shapes);
        return json;
    }

    QJsonObject convert_transform(Transform* tf, model::AnimatableBase* opacity)
    {
        QJsonObject json = convert_object_basic(tf);
        json["o"] = convert_animated(
            opacity,
            [](const QVariant& v) -> QVariant { return v.toFloat() * 100;}
        );
        return json;
    }

    QJsonValue value_from_variant(const QVariant& v)
    {
        if ( v.userType() == QMetaType::QPointF )
        {
            auto vv = v.toPointF();
            return QJsonArray{vv.x(), vv.y()};
        }
        else if ( v.userType() == QMetaType::QVector2D )
        {
            auto vv = v.value<QVector2D>() * 100;
            return QJsonArray{vv.x(), vv.y()};
        }
        else if ( v.userType() == QMetaType::QSizeF )
        {
            auto vv = v.toSizeF();
            return QJsonArray{vv.width(), vv.height()};
        }
        else if ( v.userType() == QMetaType::QColor )
        {
            auto vv = v.value<QColor>().toRgb();
            return QJsonArray{vv.redF(), vv.greenF(), vv.blueF(), vv.alphaF()};
        }
        return QJsonValue::fromVariant(v);
    }

    QJsonObject convert_object_basic(model::Object* obj)
    {
        QJsonObject json_obj;
        for ( const QMetaObject* mo = obj->metaObject(); mo; mo = mo->superClass() )
            convert_object_properties(obj, fields[model::detail::naked_type_name(mo)], json_obj);
        return json_obj;
    }

    void convert_object_properties(model::Object* obj, const QVector<FieldInfo>& fields, QJsonObject& json_obj)
    {
        for ( const auto& field : fields )
        {
            if ( field.mode != Auto )
                continue;

            model::BaseProperty * prop = obj->get_property(field.name);
            if ( !prop )
            {
                logger.stream() << field.name << "is not a property";
                continue;
            }

            if ( prop->traits().flags & PropertyTraits::Animated )
            {
                json_obj[field.lottie] = convert_animated(static_cast<AnimatableBase*>(prop));
            }
            else
            {
                json_obj[field.lottie] = value_from_variant(prop->value());
            }
        }
    }

    QJsonObject convert_animated(
        AnimatableBase* prop,
        const std::function<QVariant (const QVariant&)>& transform_values = {}
    )
    {
        /// @todo for position fields also add spatial bezier handles
        QJsonObject jobj;
        if ( prop->animated() )
        {
            jobj["a"] = 1;
            QJsonArray keyframes;
            for ( int i = 0, e = prop->keyframe_count(); i < e; i++ )
            {
                auto kf = prop->keyframe(i);
                QJsonObject jkf;
                jkf["t"] = kf->time();
                QVariant v = kf->value();
                if ( transform_values )
                    v = transform_values(v);
                jkf["s"] = value_from_variant(v);
                jkf["i"] = keyframe_bezier_handle(kf->transition().before_handle());
                jkf["o"] = keyframe_bezier_handle(kf->transition().after_handle());
                jkf["h"] = kf->transition().hold() ? 1 : 0;
                keyframes.push_back(jkf);
            }
            jobj["k"] = keyframes;
        }
        else
        {
            jobj["a"] = 0;
                QVariant v = prop->value();
                if ( transform_values )
                    v = transform_values(v);
            jobj["k"] = value_from_variant(v);
        }
        return jobj;
    }

    QJsonObject keyframe_bezier_handle(const QPointF& p)
    {
        QJsonObject jobj;
        jobj["x"] = p.x();
        jobj["y"] = p.y();
        return jobj;
    }


    QJsonObject convert_shape(model::ShapeElement* shape)
    {
        QJsonObject jsh = convert_object_basic(shape);
        jsh["ty"] = shape_types[shape->type_name()];
//         jsh["d"] = 0;
        if ( shape->type_name() == "Group" )
        {
            auto gr = static_cast<model::Group*>(shape);
            auto shapes = convert_shapes(gr->shapes);
            auto transform = convert_transform(gr->transform.get(), &gr->opacity);
            transform["ty"] = "tr";
            shapes.push_back(transform);
            jsh["it"] = shapes;
        }
        else if ( shape->type_name() == "Fill" )
        {
            auto fill = static_cast<model::Fill*>(shape);
            jsh["r"] = int(fill->fill_rule.get());
            jsh["o"] = convert_animated(
                &fill->opacity,
                [](const QVariant& v) -> QVariant { return v.toFloat() * 100;}
            );
        }
        else if ( shape->type_name() == "Stroke" )
        {
            auto str = static_cast<model::BaseStroke*>(shape);
            switch ( str->cap.get() )
            {
                case model::BaseStroke::ButtCap:  jsh["lc"] = 1; break;
                case model::BaseStroke::RoundCap: jsh["lc"] = 2; break;
                case model::BaseStroke::SquareCap:jsh["lc"] = 3; break;
            }
            switch ( str->join.get() )
            {
                case model::BaseStroke::MiterJoin: jsh["lj"] = 1; break;
                case model::BaseStroke::RoundJoin: jsh["lj"] = 2; break;
                case model::BaseStroke::BevelJoin: jsh["lj"] = 3; break;
            }
            jsh["o"] = convert_animated(
                &str->opacity,
                [](const QVariant& v) -> QVariant { return v.toFloat() * 100;}
            );
        }

        return jsh;
    }

    QJsonArray convert_shapes(const ShapeListProperty& shapes)
    {
        QJsonArray jshapes;
        for ( const auto& shape : shapes )
            jshapes.push_back(convert_shape(shape.get()));
        return jshapes;
    }

    model::Document* document;
    QMap<QUuid, int> layer_indices;
    app::log::Log logger{"Lottie Export"};

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
        load_composition(json, document->main_composition());
    }

private:
    void load_composition(const QJsonObject& json, model::Composition* composition)
    {
        this->composition = composition;
        invalid_indices.clear();
        layer_indices.clear();
        deferred.clear();

        load_basic(json, composition);
        for ( const auto& layer : json["layers"].toArray() )
            create_layer(layer.toObject());

        auto deferred_layers = std::move(deferred);
        deferred.clear();
        for ( const auto& pair: deferred_layers )
            load_layer(pair.second, static_cast<Layer*>(pair.first));
    }

    void create_layer(const QJsonObject& json)
    {
        int index = json["ind"].toInt();
        if ( !json.contains("ty") || !json["ty"].isDouble() )
        {
            emit format->error(QObject::tr("Missing layer type for %1").arg(index));
            invalid_indices.insert(index);
            return;
        }

        QString type = layer_types.key(json["ty"].toInt());
        if ( type.isEmpty() )
        {
            emit format->error(QObject::tr("Unsupported layer type %1").arg(json["ty"].toInt()));
            invalid_indices.insert(index);
            return;
        }

        model::Layer* layer = model::Factory::instance().make_layer(type, document, composition);
        if ( !layer )
        {
            emit format->error(QObject::tr("Unsupported layer type %1").arg(json["ty"].toInt()));
            invalid_indices.insert(index);
            return;
        }

        layer_indices[layer_indices.size()] = layer;
        deferred.emplace_back(layer, json);
        composition->add_layer(std::unique_ptr<Layer>(layer), composition->docnode_child_count());
    }

    void load_layer(const QJsonObject& json, model::Layer* layer)
    {
        load_basic(json, layer);

        if ( json.contains("parent") )
        {
            int parent_index = json["parent"].toInt();
            if ( invalid_indices.count(parent_index) )
            {
                emit format->error(
                    QObject::tr("Cannot use %1 as parent as it couldn't be loaded")
                    .arg(parent_index)
                );
            }
            else
            {
                auto it = layer_indices.find(parent_index);
                if ( it == layer_indices.end() )
                {
                    emit format->error(
                        QObject::tr("Invalid parent layer %1")
                        .arg(parent_index)
                    );
                }
                else
                {
                    layer->parent.set(*it);
                }
            }
        }

        load_basic(json["ks"].toObject(), layer->transform.get());


        if ( layer->type_name() == "SolidColorLayer" )
            static_cast<SolidColorLayer*>(layer)->color.set(QColor(json["sc"].toString()));
    }

    void load_basic(const QJsonObject& json_obj, model::Object* obj)
    {
        std::set<QString> props;

        for ( auto it = json_obj.begin(); it != json_obj.end(); ++it )
            props.insert(it.key());

        for ( const QMetaObject* mo = obj->metaObject(); mo; mo = mo->superClass() )
            load_properties(
                obj,
                fields[model::detail::naked_type_name(mo)],
                json_obj,
                props
            );

        for ( const auto& not_found : props )
            emit format->error(QObject::tr("Unknown field %1").arg(not_found));
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

            if ( prop->traits().flags & PropertyTraits::Animated )
            {
                load_animated(static_cast<AnimatableBase*>(prop), json_obj[field.lottie]);
            }
            else
            {
                load_value(prop, json_obj[field.lottie]);
            }
        }
    }

    template<class T>
    std::optional<QVariant> compound_value_2d(const QJsonValue& val, double mul = 1)
    {
        QJsonArray arr = val.toArray();
        if ( arr.size() < 2 || !arr[0].isDouble() || !arr[1].isDouble() )
            return {};

        return QVariant::fromValue(T(arr[0].toDouble() * mul, arr[1].toDouble() * mul));
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
            default:
                logger.stream(app::log::Error) << "Unsupported type" << prop->traits().type << "for" << prop->name();
                return {};
        }
    }

    void load_value(model::BaseProperty * prop, const QJsonValue& val)
    {
        auto v = value_to_variant(prop, val);
        if ( !v || !prop->set_value(*v) )
            emit format->error(QObject::tr("Invalid value for %1").arg(prop->name()));
    }

    void load_animated(model::AnimatableBase* prop, const QJsonValue& val)
    {
        if ( !val.isObject() )
        {
            emit format->error(QObject::tr("Invalid value for %1").arg(prop->name()));
            return;
        }

        QJsonObject obj = val.toObject();
        if ( !obj.contains("a") || !obj.contains("k") )
        {
            emit format->error(QObject::tr("Invalid value for %1").arg(prop->name()));
            return;
        }

        if ( obj["a"].toInt() )
        {
            if ( !obj["k"].isArray() )
            {
                emit format->error(QObject::tr("Invalid keyframes for %1").arg(prop->name()));
                return;
            }

            /// @todo for position fields also add spatial bezier handles
            for ( const QJsonValue& jkf : obj["k"].toArray() )
            {
                model::FrameTime time = jkf["t"].toDouble();
                auto v = value_to_variant(prop, jkf["s"]);
                model::KeyframeBase* kf = nullptr;
                if ( v )
                    kf = prop->set_keyframe(time, *v);

                if ( kf )
                {
                    kf->transition().set_before_handle(keyframe_bezier_handle(jkf["i"]));
                    kf->transition().set_after_handle(keyframe_bezier_handle(jkf["o"]));

                    if ( jkf["h"].toInt() )
                        kf->transition().set_hold(true);
                }
                else
                {
                    emit format->error(QObject::tr("Cannot load keyframe at %1 for %2")
                        .arg(time).arg(prop->name())
                    );
                }
            }
        }
        else
        {
            load_value(prop, obj["k"]);
        }
    }

    QPointF keyframe_bezier_handle(const QJsonValue& val)
    {
        QJsonObject jobj = val.toObject();
        return {jobj["x"].toDouble(), jobj["y"].toDouble()};
    }

    model::Document* document;
    io::lottie::LottieFormat* format;
    QMap<int, model::Layer*> layer_indices;
    std::set<int> invalid_indices;
    std::vector<std::pair<Object*, QJsonObject>> deferred;
    model::Composition* composition = nullptr;
    app::log::Log logger{"Lottie Import"};
};

} // namespace

bool io::lottie::LottieFormat::on_save(QIODevice& file, const QString&,
                                         model::Document* document, const QVariantMap& setting_values)
{
    file.write(to_json(document).toJson(setting_values["pretty"].toBool() ? QJsonDocument::Indented : QJsonDocument::Compact));
    return true;
}

QJsonDocument io::lottie::LottieFormat::to_json(model::Document* document)
{
    LottieExporterState exp(document);
    return QJsonDocument(exp.to_json());
}

bool io::lottie::LottieFormat::on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&)
{
    QJsonDocument jdoc;

    try {
        jdoc = QJsonDocument::fromJson(file.readAll());
    } catch ( const QJsonParseError& err ) {
        emit error(tr("Could not parse JSON: %1").arg(err.errorString()));
        return false;
    }

    if ( !jdoc.isObject() )
    {
        emit error(tr("No JSON object found"));
        return false;
    }

    QJsonObject top_level = jdoc.object();

    LottieImporterState imp{document, this};
    imp.load(top_level);
    return true;
}
