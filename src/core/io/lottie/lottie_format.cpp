#include "lottie_format.hpp"

#include <set>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCborValue>
#include <QCborArray>

#include "app/log/log.hpp"
#include "model/shapes/shapes.hpp"
#include "math/bezier/bezier.hpp"
#include "cbor_write_json.hpp"
#include <QDebug>

using namespace model;

io::Autoreg<io::lottie::LottieFormat> io::lottie::LottieFormat::autoreg;

namespace  {

class ValueTransform
{
public:
    virtual ~ValueTransform() {}

    virtual QVariant to_lottie(const QVariant& v, FrameTime) const = 0;
    virtual QVariant from_lottie(const QVariant& v, FrameTime) const = 0;
};

class FloatMult : public ValueTransform
{
public:
    explicit FloatMult(float factor) : factor(factor) {}

    QVariant to_lottie(const QVariant& v, FrameTime) const override
    {
        return v.toFloat() * factor;
    }

    QVariant from_lottie(const QVariant& v, FrameTime) const override
    {
        return v.toFloat() / factor;
    }
private:
    float factor;
};

class EnumMap : public ValueTransform
{
public:
    EnumMap(QMap<int, int> values) : values(std::move(values)) {}

    QVariant to_lottie(const QVariant& v, FrameTime) const override
    {
        return values[v.toInt()];
    }

    QVariant from_lottie(const QVariant& v, FrameTime) const override
    {
        return values.key(v.toInt());
    }

    QMap<int, int> values;
};

class GradientLoad : public ValueTransform
{
public:
    GradientLoad(int count) : count(count) {}

    QVariant to_lottie(const QVariant&, FrameTime) const override { return {}; }

    QVariant from_lottie(const QVariant& v, FrameTime) const override
    {
        auto vlist = v.toList();
        if ( vlist.size() < count * 4 )
            return {};

        QGradientStops s;
        s.reserve(count);
        bool alpha = vlist.size() >= count * 5;
        for ( int i = 0; i < count; i++ )
        {
            s.push_back({
                vlist[i*4].toDouble(),
                QColor::fromRgbF(
                    vlist[i*4+1].toDouble(),
                    vlist[i*4+2].toDouble(),
                    vlist[i*4+3].toDouble(),
                    alpha ? vlist[count*4+i].toDouble() : 1
                )
            });
        }

        return QVariant::fromValue(s);
    }

    int count = 0;
};


class TransformFunc
{
public:
    template<class T, class = std::enable_if_t<std::is_base_of_v<ValueTransform, T>>>
    TransformFunc(const T& t) : trans(std::make_shared<T>(t)) {}

    TransformFunc() {}
    TransformFunc(TransformFunc&&) = default;
    TransformFunc(const TransformFunc&) = default;

    QVariant to_lottie(const QVariant& v, FrameTime t) const
    {
        if ( !trans )
            return v;
        return trans->to_lottie(v, t);
    }

    QVariant from_lottie(const QVariant& v, FrameTime t) const
    {
        if ( !trans )
            return v;
        return trans->from_lottie(v, t);
    }

private:
    std::shared_ptr<ValueTransform> trans;
};

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
    TransformFunc transform;

    FieldInfo(const char* lottie, const char* name, TransformFunc transform = {}, bool essential = true)
        : name(name), lottie(lottie), essential(essential), mode(Auto), transform(std::move(transform))
    {}

    FieldInfo(const char* lottie, FieldMode mode = Ignored)
        : lottie(lottie), essential(false), mode(mode)
    {}
};

// static mapping data
const QMap<QString, QVector<FieldInfo>> fields = {
    {"ReferenceTarget", {
        FieldInfo{"nm", "name", {}, false},
        FieldInfo{"mn", "uuid", {}, false},
    }},
    {"Composition", {
        FieldInfo("layers", Custom),
    }},
    {"Precomposition", {
        FieldInfo("id", Custom)
    }},
    {"MainComposition", {
        FieldInfo{"op", Custom},
        FieldInfo{"ip", Custom},
        FieldInfo("v", Custom),
        FieldInfo{"fr", "fps"},
        FieldInfo{"w", "width"},
        FieldInfo{"h", "height"},
        FieldInfo("ddd"),
        FieldInfo("assets"),
        FieldInfo("comps"),
        FieldInfo("fonts"),
        FieldInfo("chars"),
        FieldInfo("markers"),
        FieldInfo("motion_blur"),
        FieldInfo("tgs"),
    }},
    // Layer is converted explicitly
    {"__Layer__", {
        FieldInfo{"op", Custom},
        FieldInfo{"ip", Custom},
        FieldInfo("ddd"),
        FieldInfo("hd"),
        FieldInfo("ty", Custom),
        FieldInfo("parent", Custom),
        FieldInfo("sr"),
        FieldInfo("ks", Custom),
        FieldInfo("ao"),
        FieldInfo{"st", "start_time"},
        FieldInfo("bm"),
        FieldInfo("tt"),
        FieldInfo("ind", Custom),
        FieldInfo("cl"),
        FieldInfo("ln"),
        FieldInfo("hasMasks"),
        FieldInfo("masksProperties"),
        FieldInfo("ef"),
    }},
    {"Transform", {
        FieldInfo{"a", "anchor_point"},
        FieldInfo("px", Custom),
        FieldInfo("py", Custom),
        FieldInfo("pz", Custom),
        FieldInfo{"p", "position"},
        FieldInfo{"s", "scale"},
        FieldInfo{"r", "rotation"},
        FieldInfo("o"),
        FieldInfo("sk"),
        FieldInfo("sa"),
        FieldInfo("nm"),
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
        FieldInfo{"p", "position"},
        FieldInfo{"s", "size"},
        FieldInfo{"r", "rounded"},
    }},
    {"Ellipse", {
        FieldInfo{"p", "position"},
        FieldInfo{"s", "size"},
    }},
    {"Path", {
        FieldInfo{"ks", "shape"},
        FieldInfo{"ind"},
    }},
    {"PolyStar", {
        FieldInfo{"p", "position"},
        FieldInfo{"or", "outer_radius"},
        FieldInfo{"ir", "inner_radius"},
        FieldInfo{"is"},
        FieldInfo{"os"},
        FieldInfo{"r", "angle"},
        FieldInfo{"pt", "points"},
        FieldInfo{"sy", "type"},
    }},
    {"Group", {
        FieldInfo{"np"},
        FieldInfo{"it", Custom},
    }},
    {"Styler", {
        FieldInfo{"o", "opacity", FloatMult(100)},
        FieldInfo{"c", "color"},
    }},
    {"Fill", {
        FieldInfo{"r", "fill_rule", EnumMap{{
            {model::Fill::NonZero, 1},
            {model::Fill::EvenOdd, 2},
        }}},
    }},
    {"Stroke", {
        FieldInfo{"lc", "cap", EnumMap{{
            {model::Stroke::ButtCap,   1},
            {model::Stroke::RoundCap,  2},
            {model::Stroke::SquareCap, 3},
        }}},
        FieldInfo{"lj", "join", EnumMap{{
            {model::Stroke::MiterJoin, 1},
            {model::Stroke::RoundJoin, 2},
            {model::Stroke::BevelJoin, 3},
        }}},
        FieldInfo{"ml", "miter_limit"},
        FieldInfo{"w", "width"},
        FieldInfo{"d"},
    }},
    {"Bitmap", {
        FieldInfo{"h", "height"},
        FieldInfo{"w", "width"},
        FieldInfo{"id", Custom},
        FieldInfo{"p", Custom},
        FieldInfo{"u", Custom},
        FieldInfo{"e", Custom},
    }},
    {"Gradient", {
        FieldInfo{"s", "start_point"},
        FieldInfo{"e", "end_point"},
        FieldInfo{"t", "type"},
        FieldInfo{"h", Custom}, /// \todo
        FieldInfo{"a", Custom}, /// \todo
        FieldInfo{"g", Custom},
    }},
    {"PreCompLayer", {
        FieldInfo{"refId", Custom},
        FieldInfo{"w", Custom},
        FieldInfo{"h", Custom},
        FieldInfo{"ks", Custom},
        FieldInfo{"ip", Custom},
        FieldInfo{"op", Custom},
        FieldInfo{"ind", Custom},
        FieldInfo{"tm"},
    }},
};
const QMap<QString, QString> shape_types = {
    {"Rect", "rc"},
    {"PolyStar", "sr"},
    {"Ellipse", "el"},
    {"Path", "sh"},
    {"Group", "gr"},
    {"Layer", "gr"},
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

const QMap<QString, QString> shape_types_repeat = {
    {"gf", "Fill"},
    {"gs", "Stroke"},
};

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

    QCborMap wrap_layer_shape(ShapeElement* shape)
    {
        QCborMap json;
        json["ddd"_l] = 0;
        json["ty"_l] = 4;
        convert_animation_container(document->main()->animation.get(), json);
        json["st"_l] = 0;

        model::Transform tf(document);
        QCborMap transform;
        convert_transform(&tf, nullptr, transform);
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

    void convert_layer(LayerType type, ShapeElement* shape, QCborArray& output, int forced_parent = -1)
    {
        switch ( type )
        {
            case LayerType::Shape:
                output.push_front(wrap_layer_shape(shape));
                return;
            case LayerType::Image:
                output.push_front(convert_image_layer(static_cast<model::Image*>(shape)));
                return;
            case LayerType::PreComp:
                output.push_front(convert_precomp_layer(static_cast<model::PreCompLayer*>(shape)));
                return;
            case LayerType::Layer:
                break;
        }

        auto layer = static_cast<Layer*>(shape);

        if ( !layer->render.get() )
            return;

        int parent_index = layer_index(layer->parent.get());
        if ( forced_parent != -1 )
            parent_index = forced_parent;

        QCborMap json;
        json["ddd"_l] = 0;
        json["ty"_l] = 3;
        int index = layer_index(layer);
        json["ind"_l] = index;

        convert_animation_container(layer->animation.get(), json);
        convert_object_properties(layer, fields["ReferenceTarget"], json);
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
            for ( const auto& shape : layer->shapes )
            {
                children_types.push_back(layer_type(shape.get()));
                if ( children_types.back() != LayerType::Shape )
                    all_shapes = false;
            }

            if ( all_shapes )
            {
                json["ty"_l] = 4;
                json["shapes"_l] = convert_shapes(layer->shapes);
            }
            else
            {
                for ( int i = 0; i < layer->shapes.size(); i++ )
                    convert_layer(children_types[i], layer->shapes[i], output, index);
            }
        }

        output.push_front(json);
    }

    void convert_transform(Transform* tf, model::AnimatableBase* opacity, QCborMap& json)
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
            for ( const auto& stop : gradient )
            {
                weird_ass_representation.push_back(stop.first);
                weird_ass_representation.push_back(stop.second.redF());
                weird_ass_representation.push_back(stop.second.greenF());
                weird_ass_representation.push_back(stop.second.blueF());
            }
            for ( const auto& stop : gradient )
                weird_ass_representation.push_back(stop.second.alphaF());
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

            if ( prop->traits().flags & PropertyTraits::Animated )
            {
                json_obj[field.lottie] = convert_animated(static_cast<AnimatableBase*>(prop), field.transform);
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
        AnimatableBase* prop,
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
                        jkf["i"_l] = keyframe_bezier_handle(kf->transition().before());
                        jkf["o"_l] = keyframe_bezier_handle(kf->transition().after());
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
        else if ( shape->type_name() == "Stroke" )
        {
            auto str = static_cast<model::Stroke*>(shape);
            convert_styler(str, jsh);
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

    QCborArray convert_shapes(const ShapeListProperty& shapes)
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

        for ( const auto& bmp : document->defs()->images )
            assets.push_back(convert_bitmat(bmp.get()));


        for ( const auto& comp : document->defs()->precompositions )
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

    QCborMap convert_image_layer(model::Image* image)
    {
        QCborMap json;
        json["ddd"_l] = 0;
        json["ty"_l] = 2;
        convert_animation_container(document->main()->animation.get(), json);
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

    QCborMap convert_precomp_layer(model::PreCompLayer* layer)
    {
        QCborMap json;
        json["ddd"_l] = 0;
        json["ty"_l] = 0;
        json["ind"_l] = layer_index(layer);
        convert_animation_container(layer->animation.get(), json);
        json["st"_l] = 0;
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
        load_assets(json["assets"].toArray());
        load_main_composition(json, document->main());
    }

private:
    void load_animation_container(const QJsonObject& json, model::AnimationContainer* animation)
    {
        animation->first_frame.set(json["ip"].toInt());
        animation->last_frame.set(json["op"].toInt());
    }

    void load_main_composition(const QJsonObject& json, model::MainComposition* composition)
    {
        load_animation_container(json, composition->animation.get());
        load_composition(json, composition);
    }

    void load_composition(const QJsonObject& json, model::Composition* composition)
    {
        this->composition = composition;
        invalid_indices.clear();
        layer_indices.clear();
        deferred.clear();

        load_basic(json, composition);
        for ( auto layer : json["layers"].toArray() )
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
            emit format->warning(QObject::tr("Missing layer type for %1").arg(index));
            invalid_indices.insert(index);
            return;
        }

        auto layer = std::make_unique<model::Layer>(document);
        layer_indices[index] = layer.get();
        deferred.emplace_back(layer.get(), json);
        composition->shapes.insert(std::move(layer), 0);
    }

    void load_layer(const QJsonObject& json, model::Layer* layer)
    {
        auto props = load_basic_setup(json);
        props.erase("ind");

        load_animation_container(json, layer->animation.get());
        load_properties(layer, fields["ReferenceTarget"], json, props);
        load_properties(layer, fields["__Layer__"], json, props);

        if ( json.contains("parent") )
        {
            int parent_index = json["parent"].toInt();
            if ( invalid_indices.count(parent_index) )
            {
                emit format->warning(
                    QObject::tr("Cannot use %1 as parent as it couldn't be loaded")
                    .arg(parent_index)
                );
            }
            else
            {
                auto it = layer_indices.find(parent_index);
                if ( it == layer_indices.end() )
                {
                    emit format->warning(
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

        load_transform(json["ks"].toObject(), layer->transform.get(), &layer->opacity);

        switch ( json["ty"].toInt(-1) )
        {
            case 0: // precomp
            {
                auto precomp = std::make_unique<model::PreCompLayer>(document);
                precomp->composition.set(precomp_ids[json["refId"].toString()]);
                props.erase("w");
                props.erase("h");
                precomp->size.set(QSize(
                    json["w"].toInt(),
                    json["h"].toInt()
                ));
                layer->shapes.insert(std::move(precomp));
                props.erase("refId");
                break;
            }
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
                layer->shapes.insert(std::move(rect));

                auto fill = std::make_unique<model::Fill>(document);
                fill->color.set(QColor(json["sc"].toString()));
                layer->shapes.insert(std::move(fill));
                break;
            }
            case 2: // image layer
            {
                auto image = std::make_unique<model::Image>(document);
                image->image.set(bitmap_ids[json["refId"].toString()]);
                layer->shapes.insert(std::move(image));
                props.erase("refId");
                break;
            }
            case 3: // empty
                break;
            case 4: // shape
                props.erase("shapes");
                load_shapes(layer->shapes, json["shapes"].toArray());
                break;
            default:
                emit format->warning(QObject::tr("Unsupported layer type %1").arg(json["ty"].toString()));
        }

        load_basic_check(props);
    }

    void load_shapes(ShapeListProperty& shapes, const QJsonArray& jshapes)
    {
        deferred.clear();

        for ( int i = jshapes.size() - 1; i >= 0; i-- )
            create_shape(jshapes[i].toObject(), shapes);

        auto deferred_shapes = std::move(deferred);
        deferred.clear();

        for ( const auto& pair: deferred_shapes )
            load_shape(pair.second, static_cast<model::ShapeElement*>(pair.first));
    }

    void create_shape(const QJsonObject& json, ShapeListProperty& shapes)
    {
        if ( !json.contains("ty") || !json["ty"].isString() )
        {
            emit format->warning(QObject::tr("Missing shape type"));
            return;
        }

        QString type = shape_types.key(json["ty"].toString());
        if ( type.isEmpty() )
        {
            type = shape_types_repeat[json["ty"].toString()];
            if ( type.isEmpty() )
            {
                emit format->warning(QObject::tr("Unsupported shape type %1").arg(json["ty"].toString()));
                return;
            }
        }

        model::ShapeElement* shape = static_cast<model::ShapeElement*>(
            model::Factory::instance().build(type, document)
        );
        if ( !shape )
        {
            emit format->warning(QObject::tr("Unsupported shape type %1").arg(json["ty"].toString()));
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
            auto gradient = document->defs()->gradients.insert(std::make_unique<model::Gradient>(document));
            styler->use.set(gradient);
            auto colors = document->defs()->gradient_colors.insert(std::make_unique<model::GradientColors>(document));
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

            if ( prop->traits().flags & PropertyTraits::Animated )
            {
                load_animated(static_cast<AnimatableBase*>(prop), json_obj[field.lottie], field.transform);
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
                QJsonArray arr = val.toArray();
                if ( arr.size() == 3 )
                    return QVariant::fromValue(QColor::fromRgbF(
                        arr[0].toDouble(), arr[1].toDouble(), arr[2].toDouble()
                    ));
                else if ( arr.size() == 4 )
                    return QVariant::fromValue(QColor::fromRgbF(
                        arr[0].toDouble(), arr[1].toDouble(), arr[2].toDouble(), arr[3].toDouble()
                    ));
                else
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
                        keyframe_bezier_handle(jkf["i"]),
                        keyframe_bezier_handle(jkf["o"]),
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
        for ( const auto& assetv : assets )
        {
            QJsonObject asset = assetv.toObject();
            if ( asset.contains("e") && asset.contains("p") && asset.contains("w") )
                load_asset_bitmap(asset);
            else if ( asset.contains("layers") )
                load_asset_precomp(asset);
        }
    }

    void load_asset_bitmap(const QJsonObject& asset)
    {
        auto bmp = document->defs()->images.insert(std::make_unique<model::Bitmap>(document));
        bitmap_ids[asset["id"].toString()] = bmp;
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

    void load_asset_precomp(QJsonObject asset)
    {
        auto comp = document->defs()->precompositions.insert(std::make_unique<model::Precomposition>(document));
        precomp_ids[asset["id"].toString()] = comp;
        asset.remove("id");
        load_composition(asset, comp);
    }

    model::Document* document;
    io::lottie::LottieFormat* format;
    QMap<int, model::Layer*> layer_indices;
    std::set<int> invalid_indices;
    std::vector<std::pair<Object*, QJsonObject>> deferred;
    model::Composition* composition = nullptr;
    app::log::Log logger{"Lottie Import"};
    QMap<QString, model::Bitmap*> bitmap_ids;
    QMap<QString, model::Precomposition*> precomp_ids;
};

} // namespace

bool io::lottie::LottieFormat::on_save(QIODevice& file, const QString&,
                                         model::Document* document, const QVariantMap& setting_values)
{
    file.write(cbor_write_json(to_json(document), !setting_values["pretty"].toBool()));
    return true;
}

QCborMap io::lottie::LottieFormat::to_json(model::Document* document, bool strip)
{
    LottieExporterState exp(this, document, strip);
    return exp.to_json();
}

bool io::lottie::LottieFormat::load_json(const QByteArray& data, model::Document* document)
{
    QJsonDocument jdoc;

    try {
        jdoc = QJsonDocument::fromJson(data);
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

bool io::lottie::LottieFormat::on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&)
{
    return load_json(file.readAll(), document);
}
