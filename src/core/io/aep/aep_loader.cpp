/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "aep_loader.hpp"
#include "model/shapes/rect.hpp"
#include "model/shapes/ellipse.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/stroke.hpp"
#include "model/shapes/image.hpp"
#include "model/shapes/polystar.hpp"
#include "model/shapes/path.hpp"
#include "model/shapes/trim.hpp"
#include "model/shapes/offset_path.hpp"
#include "model/shapes/inflate_deflate.hpp"
#include "model/shapes/zig_zag.hpp"
#include "model/shapes/round_corners.hpp"
#include "model/shapes/repeater.hpp"
#include "model/shapes/precomp_layer.hpp"
#include "model/shapes/text.hpp"

using namespace glaxnimate::io::aep;
using namespace glaxnimate;
using glaxnimate::io::ImportExport;

static constexpr std::array<QRgb, 17> label_colors = {
    0x00000000, // None
    0xffb4393b, // Red
    0xffe2d759, // Yellow
    0xffabcbc8, // Aqua
    0xffe5bcca, // Pink
    0xffa9aac9, // Lavender
    0xffe5c19f, // Peach
    0xffb4c7b4, // Sea Foam
    0xff687fdd, // Blue
    0xff4ea350, // Green
    0xff8d3299, // Purple
    0xffe79228, // Orange
    0xff7e442c, // Brown
    0xfff371d5, // Fuchsia
    0xff43a2a4, // Cyan
    0xffa7967a, // Sandstone
    0xff203f1f // Dark Green
};

void glaxnimate::io::aep::AepLoader::load_project()
{
    for ( const auto& pair : project.assets )
        load_asset(pair.second);

    for ( const auto& comp : project.compositions )
        load_comp(*comp);
}

void glaxnimate::io::aep::AepLoader::load_asset(const glaxnimate::io::aep::FolderItem* item)
{
    if ( item->type() == FolderItem::Asset )
    {
        auto image = std::make_unique<glaxnimate::model::Bitmap>(document);
        auto asset = static_cast<const FileAsset*>(item);
        if ( asset->path.exists() )
        {
            image->filename.set(asset->path.filePath());
        }
        else
        {
            // Handle collected assets
            QFileInfo path(asset_path.filePath(asset->path.fileName()));
            if ( !path.exists() )
                warning(AepFormat::tr("External asset not found: %1").arg(asset->path.filePath()));
            else
                image->filename.set(path.filePath());
        }
        image->name.set(item->name);
        images[item->id] = image.get();
        document->assets()->images->values.insert(std::move(image));
        asset_size[item->id] = QPointF(asset->width, asset->height);
    }
    else if ( item->type() == FolderItem::Solid )
    {
        auto color = std::make_unique<glaxnimate::model::NamedColor>(document);
        auto solid = static_cast<const Solid*>(item);
        color->color.set(solid->color);
        color->name.set(solid->name);
        colors[item->id] = {color.get(), solid};
        document->assets()->colors->values.insert(std::move(color));
        asset_size[item->id] = QPointF(solid->width, solid->height);
    }
    else if ( item->type() == FolderItem::Composition )
    {
        auto aecomp = static_cast<const Composition*>(item);
        asset_size[item->id] = QPointF(aecomp->width, aecomp->height);
        auto comp = get_comp(item->id);
        comp->width.set(aecomp->width);
        comp->height.set(aecomp->height);
        comp->name.set(aecomp->name);
    }
}

void glaxnimate::io::aep::AepLoader::warning(const QString& msg)
{
    io->warning(msg);
}

void glaxnimate::io::aep::AepLoader::info(const QString& msg)
{
    io->information(msg);
}

static bool unknown_mn(glaxnimate::io::ImportExport* io, const QString& context, const QString& mn)
{
    io->information(AepFormat::tr("Unknown property \"%1\" of \"%2\"").arg(mn).arg(context));
    return true;
}


model::Composition * glaxnimate::io::aep::AepLoader::get_comp(glaxnimate::io::aep::Id id)
{
    if ( !id )
        return nullptr;

    auto& comp = comps[id];
    if ( !comp )
        comp = document->assets()->add_comp_no_undo();

    return comp;
}

struct glaxnimate::io::aep::AepLoader::CompData
{
    struct PendingLayer
    {
        model::Layer* layer;
        Id parent = 0;
        Id track_matte = 0;
    };

    void resolve()
    {
        for ( const auto& p : pending )
        {
            if ( p.parent )
                p.layer->parent.set(layers.at(p.parent));
            /// \todo track matte
        }
    }

    model::Composition* comp;
    const Composition* ae_comp;
    std::unordered_map<Id, model::Layer*> layers = {};
    std::vector<PendingLayer> pending = {};

};

void glaxnimate::io::aep::AepLoader::load_comp(const glaxnimate::io::aep::Composition& ae_comp)
{
    auto comp = get_comp(ae_comp.id);
    comp->name.set(ae_comp.name);
    comp->width.set(ae_comp.width);
    comp->height.set(ae_comp.height);
    comp->fps.set(ae_comp.framerate);
    comp->animation->first_frame.set(ae_comp.in_time);
    comp->animation->last_frame.set(ae_comp.out_time);
    comp->group_color.set(ae_comp.color);
    comp->group_color.set(label_colors[int(ae_comp.label_color)]);

    CompData data{comp, &ae_comp};
    for ( const auto& layer : ae_comp.layers )
        load_layer(*layer, data);

    data.resolve();
}

namespace {
template<class T> T convert_value(const PropertyValue& v)
{
    return std::get<T>(v.value);
}

template<> float convert_value(const PropertyValue& v)
{
    return convert_value<qreal>(v);
}

template<> int convert_value(const PropertyValue& v)
{
    return convert_value<qreal>(v);
}

template<> QPointF convert_value(const PropertyValue& v)
{
    if ( v.type() == PropertyValue::Vector2D )
        return std::get<QPointF>(v.value);
    auto p = convert_value<QVector3D>(v.value);
    return {p.x(), p.y()};
}


template<> QVector2D convert_value(const PropertyValue& v)
{
    if ( v.type() == PropertyValue::Vector2D )
    {
        auto p = std::get<QPointF>(v.value);
        return QVector2D(p.x(), p.y());
    }
    else
    {
        auto p = convert_value<QVector3D>(v.value);
        return {p.x(), p.y()};
    }
}

template<> QSizeF convert_value(const PropertyValue& v)
{
    auto p = convert_value<QPointF>(v.value);
    return {p.x(), p.y()};
}

template<> math::bezier::Bezier convert_value(const PropertyValue& v)
{
    const auto& aebez = std::get<BezierData>(v.value);
    math::bezier::Bezier bez;
    int count = aebez.points.size();

    for ( int i = 0; i < count; i += 3 )
    {
        /// \todo smooth etc?
        math::bezier::Point p(aebez.convert_point(aebez.points[i]));
        if ( i > 0 )
            p.tan_in = aebez.convert_point(aebez.points[i-1]);
        else
            p.tan_in = aebez.convert_point(aebez.points.back());

        p.tan_out = aebez.convert_point(aebez.points[i+1]);

        if ( i == count - 1 && aebez.closed && math::fuzzy_compare(bez[0].pos, p.pos) )
        {
            bez[0].tan_in = p.tan_in;
            break;
        }

        bez.push_back(p);
    }
    bez.set_closed(aebez.closed);
    return bez;
}

template<> QGradientStops convert_value(const PropertyValue& v)
{
    return convert_value<Gradient>(v).to_qt();
}

template<class T> struct DefaultConverter
{
    T operator()(const PropertyValue& v) const { return convert_value<T>(v); }
};

template<class T, class Converter=DefaultConverter<T>>
bool load_property(model::Property<T>& prop, const Property& ae_prop, const Converter& conv = {})
{
    if ( ae_prop.value.type() )
        prop.set(conv(ae_prop.value));
    else if ( !ae_prop.keyframes.empty() && ae_prop.keyframes[0].value.type() )
        prop.set(conv(ae_prop.keyframes[0].value));
    else
        return false;

    return true;
}

template<class T> void kf_extra_data(model::Keyframe<T>* kf, const Keyframe& aekf)
{
    (void)kf;
    (void)aekf;
}

template<>
void kf_extra_data(model::Keyframe<QPointF>* kf, const Keyframe& aekf)
{
    auto p = kf->get();
    kf->set_point(math::bezier::Point(
        p,
        p + aekf.in_tangent,
        p + aekf.out_tangent
    ));
}

qreal vector_length(const std::vector<double>& v)
{
    qreal len = 0;
    for ( double a : v )
        len += a * a;
    return math::sqrt(len);
}

model::KeyframeTransition keyframe_transition(const Property& prop, const Keyframe& kf, const Keyframe& next_kf)
{
    qreal duration = next_kf.time - kf.time;
    if ( qFuzzyIsNull(duration) )
        return model::KeyframeTransition(model::KeyframeTransition::Linear);

    qreal average_speed = 0;
    if ( prop.type == PropertyType::Position )
    {
        math::bezier::BezierSegment bez;
        if ( kf.value.type() == PropertyValue::Vector2D )
        {
            bez[0] = std::get<QPointF>(kf.value.value);
            bez[3] = std::get<QPointF>(next_kf.value.value);
        }
        else
        {
            auto p = std::get<QVector3D>(kf.value.value);
            bez[0] = {p.x(), p.y()};
            p = std::get<QVector3D>(next_kf.value.value);
            bez[3] = {p.x(), p.y()};
        }

        bez[1] = kf.out_tangent;
        bez[2] = kf.in_tangent;

        average_speed = math::bezier::LengthData(math::bezier::CubicBezierSolver(bez), 20).length();

    }
    else if ( prop.type == PropertyType::NoValue )
    {
        average_speed = 1;
    }
    else
    {
        average_speed = math::abs(kf.value.magnitude() - next_kf.value.magnitude());
    }

    average_speed /= duration;
    qreal out_influence = vector_length(kf.out_influence);
    qreal in_influence = vector_length(kf.in_influence);
    qreal out_speed = vector_length(kf.out_speed);
    qreal in_speed = vector_length(kf.in_speed);

    QPointF ease_out;
    QPointF ease_in;
    ease_out.setX(out_influence);
    ease_in.setX(1 - in_influence);
    if ( qFuzzyIsNull(average_speed) )
    {
        ease_out.setY(out_influence);
        ease_in.setY(1 - in_influence);
    }
    else
    {
        ease_out.setY(out_influence * out_speed / average_speed);
        ease_in.setY(1 - in_influence * in_speed / average_speed);
    }

    return model::KeyframeTransition(ease_out, ease_in);
}

template<class T, class Converter=DefaultConverter<T>>
bool load_property(
    model::AnimatedProperty<T>& prop, const Property& ae_prop, const Converter& conv = {}
)
{
    if ( !ae_prop.animated && ae_prop.value.type() )
    {
        prop.set(conv(ae_prop.value));
        return true;
    }

    for ( std::size_t i = 0; i < ae_prop.keyframes.size(); i++ )
    {
        const auto& aekf = ae_prop.keyframes[i];
        auto kf = prop.set_keyframe(aekf.time, conv(aekf.value));

        kf_extra_data(kf, aekf);

        /// \todo easing
        if ( aekf.transition_type == KeyframeTransitionType::Hold )
            kf->set_transition(model::KeyframeTransition(model::KeyframeTransition::Hold));
        else if ( aekf.transition_type == KeyframeTransitionType::Linear )
            kf->set_transition(model::KeyframeTransition(model::KeyframeTransition::Linear));
        else if ( i + 1 < ae_prop.keyframes.size() )
            kf->set_transition(keyframe_transition(ae_prop, aekf, ae_prop.keyframes[i+1]));
    }

    return true;
}

template<class PropT, class Converter=DefaultConverter<typename PropT::value_type>>
void load_property_check(
    ImportExport* io,
    PropT& prop,
    const PropertyBase& ae_prop,
    const QString& match_name,
    const Converter& conv = {}
)
{
    if ( ae_prop.class_type() != PropertyBase::Property )
    {
        io->warning(AepFormat::tr("Expected property for %1").arg(match_name));
        return;
    }

    try
    {
        if ( !load_property(prop, static_cast<const Property&>(ae_prop), conv) )
            io->warning(AepFormat::tr("Could convert %1").arg(match_name));
    }
    catch ( const std::bad_variant_access& )
    {
        io->error(AepFormat::tr("Invalid value for %1").arg(match_name));
    }
}

template<class PropT, class Converter=DefaultConverter<typename PropT::value_type>>
bool load_property(ImportExport* io, PropT& prop,
                   const PropertyPair& ae_prop, const char* match_name, const Converter& conv = {})
{
    if ( ae_prop.match_name != match_name )
        return false;

    load_property_check(io, prop, *ae_prop.value, ae_prop.match_name, conv);
    return true;
}

bool convert_shape_reverse(const PropertyValue& v)
{
    return convert_value<int>(v) == 3;
}

template<int Divisor, class T = qreal>
T convert_divide(const PropertyValue& v)
{
    return convert_value<T>(v) / Divisor;
}

template<class T>
T convert_enum(const PropertyValue& v)
{
    return T(convert_value<int>(v));
}

template<>
model::Fill::Rule convert_enum(const PropertyValue& v)
{
    if ( convert_value<int>(v) == 2 )
        return model::Fill::Rule::EvenOdd;
    return model::Fill::Rule::NonZero;
}

template<>
model::Stroke::Cap convert_enum(const PropertyValue& v)
{
    switch ( convert_value<int>(v) )
    {
        default:
        case 1: return model::Stroke::Cap::ButtCap;
        case 2: return model::Stroke::Cap::RoundCap;
        case 3: return model::Stroke::Cap::SquareCap;
    }
}

template<>
model::Stroke::Join convert_enum(const PropertyValue& v)
{
    switch ( convert_value<int>(v) )
    {
        default:
        case 1: return model::Stroke::Join::MiterJoin;
        case 2: return model::Stroke::Join::RoundJoin;
        case 3: return model::Stroke::Join::BevelJoin;
    }
}

struct AnchorMult
{
    QPointF operator()(const PropertyValue& v) const
    {
        auto a = convert_value<QPointF>(v);
        return {a.x() * p.x(), a.y() * p.y()};
    }
    QPointF p;
};

void load_transform(io::ImportExport* io, model::Transform* tf, const PropertyBase& prop, model::AnimatedProperty<float>* opacity, const QPointF& anchor_mult, bool divide_100)
{
    if ( prop.class_type() != PropertyBase::PropertyGroup )
    {
        io->warning(AepFormat::tr("Expected property group for transform"));
        return;
    }

    const PropertyGroup& g = static_cast<const PropertyGroup&>(prop);
    if ( g.split_position )
    {
        /// \todo
        io->warning(AepFormat::tr("Split position currently not supported"));
    }

    bool is_3d = false;

    for ( const auto& p : g.properties )
    {
        if ( p.match_name.endsWith("Anchor Point") || p.match_name.endsWith("Anchor") )
            load_property_check(io, tf->anchor_point, *p.value, p.match_name, AnchorMult{anchor_mult});
        else if ( p.match_name.endsWith("Position") )
            load_property_check(io, tf->position, *p.value, p.match_name);
        else if ( p.match_name.endsWith("Scale") )
            load_property_check(io, tf->scale, *p.value, p.match_name, divide_100 ? &convert_divide<100, QVector2D> : &convert_divide<1, QVector2D>);
        else if ( p.match_name.endsWith("Rotation") || p.match_name.endsWith("Rotate Z") )
            load_property_check(io, tf->rotation, *p.value, p.match_name);
        else if ( opacity && p.match_name.endsWith("Opacity") )
            load_property_check(io, *opacity, *p.value, p.match_name, divide_100 ? &convert_divide<100> : &convert_divide<1>);
        else if (
            p.match_name.endsWith("Rotate X") ||
            p.match_name.endsWith("Rotate Y") ||
            p.match_name.endsWith("Orientation") ||
            p.match_name.endsWith("Position_2")
        )
            is_3d = true;
        else if ( !p.match_name.endsWith("Position_1") &&
            !p.match_name.endsWith("Position_0") &&
            !p.match_name.endsWith("Opacity") &&
            !p.match_name.endsWith("Envir Appear in Reflect")
        )
            io->information(AepFormat::tr("Unknown property \"%1\"").arg(p.match_name));
    }

    if ( is_3d )
    {
        /// \todo figure a way of determining whether the transform is actually 3D
        /// as layer transfoms seem to often have the 3D properties
        (void)is_3d;
//         warning(AepFormat::tr("3D transforms are not supported"));
    }
}

template<class Obj>
struct PropertyConverterBase
{
    virtual ~PropertyConverterBase() noexcept = default;

    virtual void load(ImportExport* io, Obj* object, const PropertyBase& ae_prop) const = 0;
    virtual void set_default(Obj* object) const = 0;
};

template<class Obj, class Base, class PropT, class T = typename PropT::value_type, class Converter=DefaultConverter<T>>
struct PropertyConverter : PropertyConverterBase<Obj>
{
    PropertyConverter(PropT (Base::*prop), const char* match_name, const Converter& converter, const std::optional<T>& default_value = {})
        : prop(prop), match_name(match_name), converter(converter), default_value(default_value)
    {}

    void load(ImportExport* io, Obj* object, const PropertyBase& ae_prop) const override
    {
        load_property_check(io, object->*prop, ae_prop, match_name, converter);
    }

    void set_default(Obj* object) const override
    {
        if ( default_value )
            (object->*prop).set(*default_value);
    }

    PropT Base::*prop;
    QString match_name;
    Converter converter = {};
    std::optional<T> default_value ;
};

template<class Base>
struct ObjectConverterBase
{
    virtual ~ObjectConverterBase() noexcept = default;
    virtual std::unique_ptr<Base> load(ImportExport* io, model::Document* document, const PropertyPair& prop) const = 0;
};

template<class Obj, class Base, class FuncT>
struct ObjectConverterFunctor : public ObjectConverterBase<Base>
{
    template<class F>
    ObjectConverterFunctor(F&& functor) : functor(std::forward<F>(functor)) {}

    std::unique_ptr<Base> load(ImportExport* io, model::Document* document, const PropertyPair& prop) const override
    {
        return functor(io, document, prop);
    }

    FuncT functor;
};

struct FallbackConverterBase
{
    virtual ~FallbackConverterBase() noexcept = default;
    virtual void set_default() const = 0;
    virtual void load_property(ImportExport* io, model::Document* document, const PropertyPair& prop_parent, const PropertyPair& prop) const = 0;
};

template<class Obj, class Base> struct FallbackConverter;


template<class Obj, class Base>
struct ObjectConverter : public ObjectConverterBase<Base>
{
    std::unique_ptr<Base> load(ImportExport* io, model::Document* document, const PropertyPair& prop) const override
    {
        return load_object(io, document, prop);
    }

    void set_default(Obj* object, FallbackConverterBase* fallback) const
    {
        for ( const auto& conv : converters )
            if ( conv.second )
                conv.second->set_default(object);

        if ( fallback )
            fallback->set_default();
    }

    void load_property(Obj* object, ImportExport* io, model::Document* document, const PropertyPair& prop_parent, const PropertyPair& prop, FallbackConverterBase* fallback) const
    {
        auto it = converters.find(prop.match_name);
        if ( it == converters.end() )
        {
            if ( fallback )
                fallback->load_property(io, document, prop_parent, prop);
            else
                unknown_mn(io, prop_parent.match_name, prop.match_name);
        }
        else if ( it->second )
        {
            it->second->load(io, object, *prop.value);
        }
    }

    void load_properties(Obj* object, ImportExport* io, model::Document* document, const PropertyPair& prop, FallbackConverterBase* fallback = nullptr) const
    {
        set_default(object, fallback);

        for ( const auto& p : *prop.value )
            this->load_property(object, io, document, prop, p, fallback);
    }

    std::unique_ptr<Obj> load_object(ImportExport* io, model::Document* document, const PropertyPair& prop) const
    {
        auto object = std::make_unique<Obj>(document);
        load_properties(object.get(), io, document, prop);
        return object;
    }

    template<class O2, class PropT, class T = typename PropT::value_type, class Converter=DefaultConverter<T>>
    ObjectConverter& prop(PropT O2::* property, const char* match_name, const Converter& conv = {})
    {
        auto ptr = std::make_unique<PropertyConverter<Obj, O2, PropT, T, Converter>>(property, match_name, conv);
        converters.emplace(match_name, std::move(ptr));
        return *this;
    }

    template<class O2, class PropT, class T = typename PropT::value_type, class Converter=DefaultConverter<T>>
    ObjectConverter& prop(PropT O2::*property, const char* match_name, const Converter& conv, const T& default_value)
    {
        converters.emplace(match_name, std::make_unique<PropertyConverter<Obj, O2, PropT, T, Converter>>(property, match_name, conv, default_value));
        return *this;
    }

    ObjectConverter& ignore(const char* match_name)
    {
        converters.emplace(match_name, nullptr);
        return *this;
    }

    FallbackConverter<Obj, Base> fallback(Obj* object, FallbackConverterBase* next) const
    {
        return {object, this, next};
    }

    std::unordered_map<QString, std::unique_ptr<PropertyConverterBase<Obj>>> converters;
};


template<class Obj, class Base>
struct FallbackConverter : public FallbackConverterBase
{
    FallbackConverter(Obj* object, const ObjectConverter<Obj, Base>* converter, FallbackConverterBase* next)
        : object(object), converter(converter), next(next)
    {}

    void set_default() const override
    {
        converter->set_default(object, next);
    }

    void load_property(ImportExport* io, model::Document* document, const PropertyPair& prop_parent, const PropertyPair& prop) const override
    {
        converter->load_property(object, io, document, prop_parent, prop, next);
    }

    Obj* object;
    const ObjectConverter<Obj, Base>* converter;
    FallbackConverterBase* next;
};

template<class Base>
struct ObjectFactory
{
    std::unique_ptr<Base> load(ImportExport* io, model::Document* document, const PropertyPair& prop) const
    {
        auto it = converters.find(prop.match_name);
        if ( it == converters.end() )
            return {};
        return it->second->load(io, document, prop);
    }

    template<class Obj, class FuncT>
    void obj(const char* match_name, FuncT&& func)
    {
        assert(converters.count(match_name) == 0);
        auto up = std::make_unique<ObjectConverterFunctor<Obj, Base, std::decay_t<FuncT>>>(std::forward<FuncT>(func));
        converters.emplace(match_name, std::move(up));
    }

    template<class Obj>
    ObjectConverter<Obj, Base>& obj(const char* match_name)
    {
        assert(converters.count(match_name) == 0);
        auto up = std::make_unique<ObjectConverter<Obj, Base>>();
        auto ptr = up.get();
        converters.emplace(match_name, std::move(up));
        return *ptr;
    }

    std::unordered_map<QString, std::unique_ptr<ObjectConverterBase<Base>>> converters;
};


std::unique_ptr<model::ShapeElement> create_shape(ImportExport* io, model::Document* document, const PropertyPair& prop);

std::unique_ptr<model::ShapeElement> load_shape(ImportExport* io, model::Document* document, const PropertyPair& prop)
{
    auto shape = create_shape(io, document, prop);
    if ( shape && prop.value->class_type() == PropertyBase::PropertyGroup )
    {
        const auto& gp = static_cast<const PropertyGroup&>(*prop.value);
        shape->visible.set(gp.visible);
    }
    return shape;
}

const ObjectConverter<model::Gradient, model::Gradient>& gradient_converter()
{
    static ObjectConverter<model::Gradient, model::Gradient> gradient;
    static bool initialized = false;
    if ( !initialized )
    {
        initialized = true;
        gradient
            .prop(&model::Gradient::type, "ADBE Vector Grad Type", &convert_enum<model::Gradient::GradientType>)
            .prop(&model::Gradient::start_point, "ADBE Vector Grad Start Pt")
            .prop(&model::Gradient::end_point, "ADBE Vector Grad End Pt")
            .ignore("ADBE Vector Grad HiLite Length") /// \todo
            .ignore("ADBE Vector Grad HiLite Angle") /// \todo
        ;
    }

    return gradient;
}

const ObjectConverter<model::GradientColors, model::GradientColors>& gradient_stop_converter()
{
    static ObjectConverter<model::GradientColors, model::GradientColors> gradient;
    static bool initialized = false;
    if ( !initialized )
    {
        initialized = true;
        gradient
            .prop(&model::GradientColors::colors, "ADBE Vector Grad Colors")
        ;
    }

    return gradient;
}

template<class T>
std::unique_ptr<model::ShapeElement> load_gradient(const ObjectConverter<T, model::ShapeElement>* base_converter, ImportExport* io, model::Document* document, const PropertyPair& prop)
{
    auto shape = std::make_unique<T>(document);
    auto grad_colors = document->assets()->gradient_colors->values.insert(
        std::make_unique<glaxnimate::model::GradientColors>(document)
    );
    auto grad = document->assets()->gradients->values.insert(
        std::make_unique<glaxnimate::model::Gradient>(document)
    );
    grad->end_point.set({100, 0}); // default value
    grad->colors.set(grad_colors);
    shape->use.set(grad);

    auto f1 = gradient_stop_converter().fallback(grad_colors, nullptr);
    auto f2 = gradient_converter().fallback(grad, &f1);
    base_converter->load_properties(shape.get(), io, document, prop, &f2);

    grad->highlight.set(grad->start_point.get());

    return shape;
}

const ObjectFactory<model::ShapeElement>& shape_factory()
{
    static ObjectFactory<model::ShapeElement> factory;
    static bool initialized = false;
    if ( !initialized )
    {
        initialized = true;
        factory.obj<model::Group>("ADBE Vector Group", [](ImportExport* io, model::Document* document, const PropertyPair& prop) {
            auto gp = std::make_unique<model::Group>(document);
            load_transform(io, gp->transform.get(), (*prop.value)["ADBE Vector Transform Group"], &gp->opacity, {1, 1}, true);

            for ( const auto& prop : (*prop.value)["ADBE Vectors Group"] )
            {
                if ( auto shape = load_shape(io, document, prop) )
                    gp->shapes.insert(std::move(shape), 0);
            }

            return gp;
        });
        factory.obj<model::Rect>("ADBE Vector Shape - Rect")
            .prop(&model::Rect::reversed, "ADBE Vector Shape Direction", &convert_shape_reverse)
            .prop(&model::Rect::position, "ADBE Vector Rect Position")
            .prop(&model::Rect::size, "ADBE Vector Rect Size")
            .prop(&model::Rect::rounded, "ADBE Vector Rect Roundness")
        ;
        factory.obj<model::Ellipse>("ADBE Vector Shape - Ellipse")
            .prop(&model::Ellipse::reversed, "ADBE Vector Shape Direction", &convert_shape_reverse)
            .prop(&model::Ellipse::position, "ADBE Vector Ellipse Position")
            .prop(&model::Ellipse::size, "ADBE Vector Ellipse Size")
        ;
        factory.obj<model::PolyStar>("ADBE Vector Shape - Star")
            .prop(&model::PolyStar::reversed, "ADBE Vector Shape Direction", &convert_shape_reverse)
            .prop(&model::PolyStar::position, "ADBE Vector Star Position")
            .prop(&model::PolyStar::type, "ADBE Vector Star Type", &convert_enum<model::PolyStar::StarType>)
            .prop(&model::PolyStar::points, "ADBE Vector Star Points")
            .prop(&model::PolyStar::angle, "ADBE Vector Star Rotation")
            .prop(&model::PolyStar::inner_radius, "ADBE Vector Star Inner Radius")
            .prop(&model::PolyStar::outer_radius, "ADBE Vector Star Outer Radius")
            .prop(&model::PolyStar::inner_roundness, "ADBE Vector Star Inner Roundess", &convert_divide<100>)
            .prop(&model::PolyStar::outer_roundness, "ADBE Vector Star Outer Roundess", &convert_divide<100>)
        ;
        factory.obj<model::Path>("ADBE Vector Shape - Group")
            .prop(&model::Path::reversed, "ADBE Vector Shape Direction", &convert_shape_reverse)
            .prop(&model::Path::shape, "ADBE Vector Shape")
        ;
        const auto* fill = &factory.obj<model::Fill>("ADBE Vector Graphic - Fill")
            .ignore("ADBE Vector Blend Mode")
            .prop(&model::Fill::color, "ADBE Vector Fill Color", {}, QColor(255, 0, 0))
            .prop(&model::Fill::opacity, "ADBE Vector Fill Opacity", &convert_divide<100>)
            .prop(&model::Fill::fill_rule, "ADBE Vector Fill Rule", &convert_enum<model::Fill::Rule>)
            .ignore("ADBE Vector Composite Order") /// \todo could be parsed
        ;
        const auto* stroke = &factory.obj<model::Stroke>("ADBE Vector Graphic - Stroke")
            .ignore("ADBE Vector Blend Mode")
            .prop(&model::Stroke::color, "ADBE Vector Stroke Color", {}, QColor(255, 255, 255))
            .prop(&model::Stroke::opacity, "ADBE Vector Stroke Opacity", &convert_divide<100>)
            .prop(&model::Stroke::width, "ADBE Vector Stroke Width", {}, 2)
            .prop(&model::Stroke::cap, "ADBE Vector Stroke Line Cap", &convert_enum<model::Stroke::Cap>)
            .prop(&model::Stroke::join, "ADBE Vector Stroke Line Join", &convert_enum<model::Stroke::Join>)
            .prop(&model::Stroke::miter_limit, "ADBE Vector Stroke Miter Limit")
            .ignore("ADBE Vector Stroke Dashes")
            .ignore("ADBE Vector Stroke Taper")
            .ignore("ADBE Vector Stroke Wave")
            .ignore("ADBE Vector Composite Order") /// \todo could be parsed
        ;
        factory.obj<model::RoundCorners>("ADBE Vector Filter - RC")
            .prop(&model::RoundCorners::radius, "ADBE Vector RoundCorner Radius")
        ;
        factory.obj<model::Trim>("ADBE Vector Filter - Trim")
            .prop(&model::Trim::start, "ADBE Vector Trim Start", &convert_divide<100>)
            .prop(&model::Trim::end, "ADBE Vector Trim End", &convert_divide<100>)
            .prop(&model::Trim::offset, "ADBE Vector Trim Offset", &convert_divide<360>)
        ;
        factory.obj<model::OffsetPath>("ADBE Vector Filter - Offset")
            .prop(&model::OffsetPath::amount, "ADBE Vector Offset Amount")
            .prop(&model::OffsetPath::join, "ADBE Vector Offset Line Join", &convert_enum<model::Stroke::Join>)
            .prop(&model::OffsetPath::miter_limit, "ADBE Vector Offset Miter Limit")
        ;
        factory.obj<model::InflateDeflate>("ADBE Vector Filter - PB")
            .prop(&model::InflateDeflate::amount, "ADBE Vector PuckerBloat Amount")
        ;
        factory.obj<model::ZigZag>("ADBE Vector Filter - Zigzag")
            .prop(&model::ZigZag::amplitude, "ADBE Vector Zigzag Size")
            .prop(&model::ZigZag::frequency, "ADBE Vector Zigzag Detail")
            .prop(&model::ZigZag::style, "ADBE Vector Zigzag Points", &convert_enum<model::ZigZag::Style>)
        ;
        factory.obj<model::Fill>("ADBE Vector Graphic - G-Fill", [fill](ImportExport* io, model::Document* document, const PropertyPair& prop) {
            return load_gradient(fill, io, document, prop);
        });
        factory.obj<model::Stroke>("ADBE Vector Graphic - G-Stroke", [stroke](ImportExport* io, model::Document* document, const PropertyPair& prop) {
            return load_gradient(stroke, io, document, prop);
        });
        factory.obj<model::Repeater>("ADBE Vector Filter - Repeater", [](ImportExport* io, model::Document* document, const PropertyPair& prop) {
            auto shape = std::make_unique<model::Repeater>(document);
            if ( auto tf = prop.value->get("ADBE Vector Repeater Transform") )
            {
                load_transform(io, shape->transform.get(), *tf, nullptr, {1, 1}, false);
                const char* pmn = "ADBE Vector Repeater Start Opacity";
                if ( auto o = tf->get(pmn) )
                    load_property_check(io, shape->start_opacity, *o, pmn, &convert_divide<100>);
                pmn = "ADBE Vector Repeater End Opacity";
                if ( auto o = tf->get(pmn) )
                    load_property_check(io, shape->end_opacity, *o, pmn, &convert_divide<100>);
            }

            if ( auto copies = prop.value->get("ADBE Vector Repeater Copies") )
            {
                load_property_check(io, shape->copies, *copies, "ADBE Vector Repeater Copies");
            }

            return shape;
        });
    }

    return factory;
};


std::unique_ptr<model::ShapeElement> create_shape(ImportExport* io, model::Document* document, const PropertyPair& prop)
{
    if ( auto shape = shape_factory().load(io, document, prop) )
        return shape;

    io->information(AepFormat::tr("Unknown shape %1").arg(prop.match_name));
    return nullptr;
}

} // namespace


void glaxnimate::io::aep::AepLoader::load_layer(const glaxnimate::io::aep::Layer& ae_layer, CompData& data)
{

    auto ulayer = std::make_unique<model::Layer>(document);
    auto layer = ulayer.get();
    data.comp->shapes.insert(std::move(ulayer), 0);
    data.layers[ae_layer.id] = layer;

    if ( ae_layer.parent_id || ae_layer.matte_id )
        data.pending.push_back({layer, ae_layer.parent_id, ae_layer.matte_id});

    layer->name.set(ae_layer.name);
    layer->render.set(!ae_layer.is_guide);
    layer->animation->first_frame.set(ae_layer.in_time);
    layer->animation->last_frame.set(ae_layer.out_time);
    layer->visible.set(ae_layer.properties.visible);
    /// \todo could be nice to toggle visibility based on solo/shy
    layer->group_color.set(label_colors[int(ae_layer.label_color)]);

    layer->transform->position.set({
        data.comp->width.get() / 2.,
        data.comp->height.get() / 2.,
    });

    QPointF anchor{1, 1};
    auto it = asset_size.find(ae_layer.asset_id);
    if ( it != asset_size.end() )
    {
        anchor = it->second;
        layer->transform->anchor_point.set(anchor / 2);
    }
    load_transform(io, layer->transform.get(), ae_layer.properties["ADBE Transform Group"], &layer->opacity, anchor, false);
    /// \todo auto-orient
    /// \todo masks "ADBE Mask Parade"

    if ( ae_layer.is_null )
        return;
    else if ( ae_layer.asset_id )
        asset_layer(layer, ae_layer, data);
    else if ( ae_layer.type == LayerType::ShapeLayer )
        shape_layer(layer, ae_layer, data);
    else if ( ae_layer.type == LayerType::TextLayer )
        text_layer(layer, ae_layer, data);
}

void glaxnimate::io::aep::AepLoader::shape_layer(model::Layer* layer, const glaxnimate::io::aep::Layer& ae_layer, glaxnimate::io::aep::AepLoader::CompData&)
{
    for ( const auto& prop : ae_layer.properties["ADBE Root Vectors Group"] )
    {
        if ( auto shape = load_shape(io, document, prop) )
            layer->shapes.insert(std::move(shape), 0);
    }
}

void glaxnimate::io::aep::AepLoader::asset_layer(
    model::Layer* layer, const Layer& ae_layer, CompData&
)
{
    auto img_it = images.find(ae_layer.asset_id);
    if ( img_it != images.end() )
    {
        auto image = std::make_unique<model::Image>(document);
        image->image.set(img_it->second);
        image->name.set(img_it->second->name.get());
        if ( layer->name.get().isEmpty() )
            layer->name.set(image->name.get());
        layer->shapes.insert(std::move(image));
        return;
    }

    auto comp_it = comps.find(ae_layer.asset_id);
    if ( comp_it != comps.end() )
    {
        /// \todo ADBE Time Remapping
        /// \todo Time stretch / start_time
        auto precomp = std::make_unique<model::PreCompLayer>(document);
        precomp->composition.set(comp_it->second);
        precomp->name.set(comp_it->second->name.get());
        precomp->size.set(comp_it->second->size());
        if ( layer->name.get().isEmpty() )
            layer->name.set(precomp->name.get());
        layer->shapes.insert(std::move(precomp));
        return;
    }

    auto solid_it = colors.find(ae_layer.asset_id);
    if ( solid_it != colors.end() )
    {
        auto fill = std::make_unique<model::Fill>(document);
        fill->color.set(solid_it->second.asset->color.get());
        fill->use.set(solid_it->second.asset);
        layer->shapes.insert(std::move(fill));

        auto rect = std::make_unique<model::Rect>(document);
        rect->size.set(QSizeF(
            solid_it->second.solid->width,
            solid_it->second.solid->height
        ));
        rect->position.set(QPointF(
            solid_it->second.solid->width / 2,
            solid_it->second.solid->height / 2
        ));
        layer->shapes.insert(std::move(rect));

        if ( layer->name.get().isEmpty() )
            layer->name.set(solid_it->second.asset->name.get());
        return;
    }

    warning(AepFormat::tr("Unknown asset type for %1").arg(ae_layer.name.isEmpty() ? "Layer" : ae_layer.name));
}

namespace {

std::unique_ptr<model::ShapeElement> text_to_shapes(
    const TextDocument& doc, const std::vector<Font>& fonts, model::Document* document
)
{
    auto group = std::make_unique<model::Group>(document);
    auto pt = std::make_unique<model::TextShape>(document);
    auto text = pt.get();
    group->shapes.insert(std::move(pt));
    text->text.set(doc.text);
    if ( !doc.character_styles.empty() )
    {
        /// \todo Style text spans
        const auto& style = doc.character_styles[0];
        /// \todo figure out weight etc
        if ( style.font_index >= 0 && style.font_index < int(fonts.size()) )
            text->font->family.set(fonts[style.font_index].family);
        text->font->size.set(style.size);

        std::unique_ptr<model::Fill> fill;
        std::unique_ptr<model::Stroke> stroke;

        if ( style.stroke_enabled )
        {
            stroke = std::make_unique<model::Stroke>(document);
            stroke->color.set(style.stroke_color);
            stroke->width.set(style.stroke_width);
        }

        /// \todo fill enabled?
        fill = std::make_unique<model::Fill>(document);
        fill->color.set(style.fill_color);

        if ( style.stroke_over_fill )
        {
            if ( fill )
                group->shapes.insert(std::move(fill));
            if ( stroke )
                group->shapes.insert(std::move(stroke));
        }
        else
        {
            if ( stroke )
                group->shapes.insert(std::move(stroke));
            if ( fill )
                group->shapes.insert(std::move(fill));
        }
    }

    return group;
}

} // namespace

void glaxnimate::io::aep::AepLoader::text_layer(model::Layer* layer, const Layer& ae_layer, CompData&)
{
    auto prop = ae_layer.properties["ADBE Text Properties"]["ADBE Text Document"];
    if ( prop.class_type() != PropertyBase::TextProperty )
        return;

    const auto& tprop = static_cast<const TextProperty&>(prop);

    if ( tprop.documents.value.type() == PropertyValue::TextDocument )
    {
        layer->shapes.insert(text_to_shapes(
            std::get<TextDocument>(tprop.documents.value.value), tprop.fonts, document
        ));
        return;
    }

    if ( tprop.documents.keyframes.empty() )
        return;

    /// \todo animated text
    const auto& kf = tprop.documents.keyframes[0];

    if ( kf.value.type() == PropertyValue::TextDocument )
    {
        layer->shapes.insert(text_to_shapes(
            std::get<TextDocument>(kf.value.value), tprop.fonts, document
        ));
        return;
    }
}

