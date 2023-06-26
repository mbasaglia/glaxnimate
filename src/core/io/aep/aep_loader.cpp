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
        auto comp = static_cast<const Composition*>(item);
        asset_size[item->id] = QPointF(comp->width, comp->height);
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

bool glaxnimate::io::aep::AepLoader::unknown_mn(const QString& context, const QString& mn)
{
    info(AepFormat::tr("Unknown property \"%1\" of \"%2\"").arg(mn).arg(context));
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
    layer->animation->first_frame.set(ae_layer.start_time);
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
    load_transform(layer->transform.get(), ae_layer.properties["ADBE Transform Group"], &layer->opacity, anchor);
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

void glaxnimate::io::aep::AepLoader::shape_layer(model::Layer* layer, const glaxnimate::io::aep::Layer& ae_layer, glaxnimate::io::aep::AepLoader::CompData& data)
{
    for ( const auto& prop : ae_layer.properties["ADBE Root Vectors Group"] )
    {
        if ( auto shape = load_shape(prop, data) )
            layer->shapes.insert(std::move(shape), 0);
    }
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
    for ( int i = 0; i < count; i += 4 )
    {
        /// \todo smooth etc?
        math::bezier::Point p(aebez.convert_point(aebez.points[i]));
        if ( i > 0 )
            p.tan_in = aebez.convert_point(aebez.points[i-1]);
        else if ( i < count - 1 )
            p.tan_out = aebez.convert_point(aebez.points[i-1]);

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

template<int Divisor>
qreal convert_divide(const PropertyValue& v)
{
    return convert_value<qreal>(v) / Divisor;
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

} // namespace

// hacky macros to minimize boilerplate and be more declarative
#define OBJ(mn, type) \
    else if ( prop.match_name == mn ) { \
        auto shape = std::make_unique<model::type>(document); \
        for ( const auto& p : *prop.value ) {

#define PROP_OF(obj, name, ...) \
    load_property(io, obj->name, p, __VA_ARGS__) ||

#define PROP(name, ...) \
    PROP_OF(shape, name, __VA_ARGS__)

#define IGNORE(name) (p.match_name == name) ||

#define END \
    unknown_mn(prop.match_name, p.match_name); } return shape; }

std::unique_ptr<model::ShapeElement> AepLoader::load_shape(const PropertyPair& prop, AepLoader::CompData& data)
{
    auto shape = create_shape(prop, data);
    if ( shape && prop.value->class_type() == PropertyBase::PropertyGroup )
    {
        const auto& gp = static_cast<const PropertyGroup&>(*prop.value);
        shape->visible.set(gp.visible);
    }
    return shape;
}

std::unique_ptr<model::ShapeElement> AepLoader::create_shape(const PropertyPair& prop, CompData& data)
{
    if ( prop.match_name == "ADBE Vector Group" )
    {
        auto gp = std::make_unique<model::Group>(document);
        load_transform(gp->transform.get(), (*prop.value)["ADBE Vector Transform Group"], &gp->opacity, {1, 1});

        for ( const auto& prop : (*prop.value)["ADBE Vectors Group"] )
        {
            if ( auto shape = load_shape(prop, data) )
                gp->shapes.insert(std::move(shape), 0);
        }

        return gp;
    }
    OBJ("ADBE Vector Shape - Rect", Rect)
        PROP(reversed, "ADBE Vector Shape Direction", &convert_shape_reverse)
        PROP(position, "ADBE Vector Rect Position")
        PROP(size, "ADBE Vector Rect Size")
        PROP(rounded, "ADBE Vector Rect Roundness")
    END
    OBJ("ADBE Vector Shape - Ellipse", Ellipse)
        PROP(reversed, "ADBE Vector Shape Direction", &convert_shape_reverse)
        PROP(position, "ADBE Vector Ellipse Position")
        PROP(size, "ADBE Vector Ellipse Size")
    END
    OBJ("ADBE Vector Shape - Star", PolyStar)
        PROP(reversed, "ADBE Vector Shape Direction", &convert_shape_reverse)
        PROP(position, "ADBE Vector Star Position")
        PROP(type, "ADBE Vector Star Type", &convert_enum<model::PolyStar::StarType>)
        PROP(points, "ADBE Vector Star Points")
        PROP(angle, "ADBE Vector Star Rotation")
        PROP(inner_radius, "ADBE Vector Star Inner Radius")
        PROP(outer_radius, "ADBE Vector Star Outer Radius")
        PROP(inner_roundness, "ADBE Vector Star Inner Roundess", &convert_divide<100>)
        PROP(outer_roundness, "ADBE Vector Star Outer Roundess", &convert_divide<100>)
    END
    OBJ("ADBE Vector Shape - Group", Path)
        PROP(reversed, "ADBE Vector Shape Direction", &convert_shape_reverse)
        PROP(shape, "ADBE Vector Shape")
    END
    OBJ("ADBE Vector Graphic - Fill", Fill)
        IGNORE("ADBE Vector Blend Mode")
        PROP(color, "ADBE Vector Fill Color")
        PROP(opacity, "ADBE Vector Fill Opacity", &convert_divide<100>)
        PROP(fill_rule, "ADBE Vector Fill Rule", &convert_enum<model::Fill::Rule>)
        IGNORE("ADBE Vector Composite Order") /// \todo could be parsed
    END
    OBJ("ADBE Vector Graphic - Stroke", Stroke)
        IGNORE("ADBE Vector Blend Mode")
        PROP(color, "ADBE Vector Stroke Color")
        PROP(opacity, "ADBE Vector Stroke Opacity", &convert_divide<100>)
        PROP(width, "ADBE Vector Stroke Width")
        PROP(cap, "ADBE Vector Stroke Line Cap", &convert_enum<model::Stroke::Cap>)
        PROP(join, "ADBE Vector Stroke Line Join", &convert_enum<model::Stroke::Join>)
        PROP(miter_limit, "ADBE Vector Stroke Miter Limit")
        IGNORE("ADBE Vector Stroke Dashes")
        IGNORE("ADBE Vector Stroke Taper")
        IGNORE("ADBE Vector Stroke Wave")
        IGNORE("ADBE Vector Composite Order") /// \todo could be parsed
    END
    else if ( prop.match_name == "ADBE Vector Graphic - G-Fill" )
    {
        auto shape = std::make_unique<model::Fill>(document);
        auto grad_colors = document->assets()->gradient_colors->values.insert(
            std::make_unique<glaxnimate::model::GradientColors>(document)
        );
        auto grad = document->assets()->gradients->values.insert(
            std::make_unique<glaxnimate::model::Gradient>(document)
        );
        grad->end_point.set({100, 0}); // default value
        grad->colors.set(grad_colors);
        shape->use.set(grad);

        for ( const auto& p : *prop.value )
        {
            IGNORE("ADBE Vector Blend Mode")
            PROP(opacity, "ADBE Vector Fill Opacity", &convert_divide<100>)
            PROP(fill_rule, "ADBE Vector Fill Rule", &convert_enum<model::Fill::Rule>)
            IGNORE("ADBE Vector Composite Order") /// \todo could be parsed

            PROP_OF(grad, type, "ADBE Vector Grad Type", &convert_enum<model::Gradient::GradientType>)
            PROP_OF(grad, start_point, "ADBE Vector Grad Start Pt")
            PROP_OF(grad, end_point, "ADBE Vector Grad End Pt")
            IGNORE("ADBE Vector Grad HiLite Length") /// \todo
            IGNORE("ADBE Vector Grad HiLite Angle") /// \todo
            PROP_OF(grad_colors, colors, "ADBE Vector Grad Colors")
            unknown_mn(prop.match_name, p.match_name);
        }

        grad->highlight.set(grad->start_point.get());

        return shape;
    }
    else if ( prop.match_name == "ADBE Vector Graphic - G-Stroke" )
    {
        auto shape = std::make_unique<model::Stroke>(document);
        auto grad_colors = document->assets()->gradient_colors->values.insert(
            std::make_unique<glaxnimate::model::GradientColors>(document)
        );
        auto grad = document->assets()->gradients->values.insert(
            std::make_unique<glaxnimate::model::Gradient>(document)
        );
        grad->end_point.set({100, 0}); // default value
        grad->colors.set(grad_colors);
        shape->use.set(grad);

        for ( const auto& p : *prop.value )
        {
            IGNORE("ADBE Vector Blend Mode")
            PROP(opacity, "ADBE Vector Stroke Opacity", &convert_divide<100>)
            PROP(width, "ADBE Vector Stroke Width")
            PROP(cap, "ADBE Vector Stroke Line Cap", &convert_enum<model::Stroke::Cap>)
            PROP(join, "ADBE Vector Stroke Line Join", &convert_enum<model::Stroke::Join>)
            PROP(miter_limit, "ADBE Vector Stroke Miter Limit")
            IGNORE("ADBE Vector Stroke Dashes")
            IGNORE("ADBE Vector Stroke Taper")
            IGNORE("ADBE Vector Stroke Wave")
            IGNORE("ADBE Vector Composite Order") /// \todo could be parsed

            PROP_OF(grad, type, "ADBE Vector Grad Type", &convert_enum<model::Gradient::GradientType>)
            PROP_OF(grad, start_point, "ADBE Vector Grad Start Pt")
            PROP_OF(grad, end_point, "ADBE Vector Grad End Pt")
            IGNORE("ADBE Vector Grad HiLite Length") /// \todo
            IGNORE("ADBE Vector Grad HiLite Angle") /// \todo
            PROP_OF(grad_colors, colors, "ADBE Vector Grad Colors")
            unknown_mn(prop.match_name, p.match_name);
        }

        grad->highlight.set(grad->start_point.get());

        return shape;
    }
    OBJ("ADBE Vector Filter - RC", RoundCorners)
        PROP(radius, "ADBE Vector RoundCorner Radius")
    END
    OBJ("ADBE Vector Filter - Trim", Trim)
        PROP(start, "ADBE Vector Trim Start", &convert_divide<100>)
        PROP(end, "ADBE Vector Trim End", &convert_divide<100>)
        PROP(offset, "ADBE Vector Trim Offset", &convert_divide<360>)
    END
    OBJ("ADBE Vector Filter - Offset", OffsetPath)
        PROP(amount, "ADBE Vector Offset Amount")
        PROP(join, "ADBE Vector Offset Line Join", &convert_enum<model::Stroke::Join>)
        PROP(miter_limit, "ADBE Vector Offset Miter Limit")
    END
    OBJ("ADBE Vector Filter - PB", InflateDeflate)
        PROP(amount, "ADBE Vector PuckerBloat Amount")
    END
    OBJ("ADBE Vector Filter - Zigzag", ZigZag)
        PROP(amplitude, "ADBE Vector Zigzag Size")
        PROP(frequency, "ADBE Vector Zigzag Detail")
        PROP(style, "ADBE Vector Zigzag Points", &convert_enum<model::ZigZag::Style>)
    END
    else if ( prop.match_name == "ADBE Vector Filter - Repeater" )
    {
        auto shape = std::make_unique<model::Repeater>(document);
        if ( auto tf = prop.value->get("ADBE Vector Repeater Transform") )
        {
            load_transform(shape->transform.get(), *tf, nullptr, {1, 1});
            const char* pmn = "ADBE Vector Repeater Start Opacity";
            if ( auto o = tf->get(pmn) )
                load_property_check(io, shape->start_opacity, *o, pmn, &convert_divide<100>);
            pmn = "ADBE Vector Repeater End Opacity";
            if ( auto o = tf->get(pmn) )
                load_property_check(io, shape->end_opacity, *o, pmn, &convert_divide<100>);
        }

        for ( const auto& p : *prop.value )
        {
            IGNORE("ADBE Vector Repeater Transform")
            PROP(copies, "ADBE Vector Repeater Copies")
            unknown_mn(prop.match_name, p.match_name);
        }

        return shape;
    }
    else
    {
        info(AepFormat::tr("Unknown shape %1").arg(prop.match_name));
        return {};
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

struct AnchorMult
{
    QPointF operator()(const PropertyValue& v) const
    {
        auto a = convert_value<QPointF>(v);
        return {a.x() * p.x(), a.y() * p.y()};
    }
    QPointF p;
};

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

void AepLoader::load_transform(model::Transform* tf, const PropertyBase& prop, model::AnimatedProperty<float>* opacity, const QPointF& anchor_mult)
{
    if ( prop.class_type() != PropertyBase::PropertyGroup )
    {
        warning(AepFormat::tr("Expected property group for transform"));
        return;
    }

    const PropertyGroup& g = static_cast<const PropertyGroup&>(prop);
    if ( g.split_position )
    {
        /// \todo
        warning(AepFormat::tr("Split position currently not supported"));
    }

    bool is_3d = false;
    for ( const auto& p : g.properties )
    {
        if ( p.match_name.endsWith("Anchor Point") || p.match_name.endsWith("Anchor") )
            load_property_check(io, tf->anchor_point, *p.value, p.match_name, AnchorMult{anchor_mult});
        else if ( p.match_name.endsWith("Position") )
            load_property_check(io, tf->position, *p.value, p.match_name);
        else if ( p.match_name.endsWith("Scale") )
            load_property_check(io, tf->scale, *p.value, p.match_name);
        else if ( p.match_name.endsWith("Rotation") || p.match_name.endsWith("Rotate Z") )
            load_property_check(io, tf->rotation, *p.value, p.match_name);
        else if ( opacity && p.match_name.endsWith("Opacity") )
            load_property_check(io, *opacity, *p.value, p.match_name);
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
            info(AepFormat::tr("Unknown property \"%1\"").arg(p.match_name));
    }

    if ( is_3d )
    {
        /// \todo figure a way of determining whether the transform is actually 3D
        /// as layer transfoms seem to often have the 3D properties
        (void)is_3d;
//         warning(AepFormat::tr("3D transforms are not supported"));
    }
}
