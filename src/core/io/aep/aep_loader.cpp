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
            QFileInfo path = asset_path.filePath(asset->path.fileName());
            if ( !path.exists() )
                warning(AepFormat::tr("External asset not found: %1").arg(asset->path.filePath()));
            else
                image->filename.set(path.filePath());
        }
        images[item->id] = image.get();
        document->assets()->images->values.insert(std::move(image));
    }
    else if ( item->type() == FolderItem::Solid )
    {
        auto color = std::make_unique<glaxnimate::model::NamedColor>(document);
        auto solid = static_cast<const Solid*>(item);
        color->color.set(solid->color);
        colors[item->id] = {color.get(), solid};
        document->assets()->colors->values.insert(std::move(color));

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
    /// \todo
//     comp->group_color.set(label_colors[int(ae_comp.label_color)]);

    CompData data{comp, &ae_comp};
    for ( const auto& layer : ae_comp.layers )
        load_layer(*layer, data);

    data.resolve();
}

void glaxnimate::io::aep::AepLoader::load_layer(const glaxnimate::io::aep::Layer& ae_layer, CompData& data)
{

    auto ulayer = std::make_unique<model::Layer>(document);
    auto layer = ulayer.get();
    data.comp->shapes.insert(std::move(ulayer));
    data.layers[ae_layer.id] = layer;

    if ( ae_layer.parent_id || ae_layer.matte_id )
        data.pending.push_back({layer, ae_layer.parent_id, ae_layer.matte_id});

    layer->name.set(ae_layer.name);
    layer->render.set(!ae_layer.is_guide);
    layer->animation->first_frame.set(ae_layer.start_time);
    layer->animation->last_frame.set(ae_layer.out_time);
    layer->visible.set(ae_layer.properties.visible);
    layer->group_color.set(label_colors[int(ae_layer.label_color)]);

    layer->transform->position.set({
        data.comp->width.get() / 2.,
        data.comp->height.get() / 2.,
    });

    load_transform(layer->transform.get(), ae_layer.properties["ADBE Transform Group"], "");
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
/*
template<class T, class Converter=DefaultConverter<T>>
T get_property_static_value(const Property& ae_prop, const T& defval = {}, const Converter& conv = {}, bool* ok = nullptr)
{
    if ( ok )
        *ok = true;

    if ( ae_prop.value.type() )
        return conv(ae_prop.value);
    else if ( !ae_prop.keyframes.empty() && ae_prop.keyframes[0].value.type() )
        return conv(ae_prop.keyframes[0].value);

    if ( ok )
        *ok = false;

    return defval;
}

template<class T, class Converter=DefaultConverter<T>>
T get_property_static_value(const PropertyBase& ae_prop, const T& defval = {}, const Converter& conv = {})
{
    if ( ae_prop.class_type() != PropertyBase::Property )
        return defval;
    return get_property_static_value(static_cast<const Property&>(ae_prop), defval, conv);
}
*/
template<class T, class Converter=DefaultConverter<T>>
bool load_static_property(model::Property<T>& prop, const Property& ae_prop, const Converter& conv = {})
{
    if ( ae_prop.value.type() )
        prop.set(conv(ae_prop.value));
    else if ( !ae_prop.keyframes.empty() && ae_prop.keyframes[0].value.type() )
        prop.set(conv(ae_prop.keyframes[0].value));
    else
        return false;

    return true;
}

template<class T, class Converter=DefaultConverter<T>>
bool load_property(
    ImportExport* io, model::Property<T>& prop, const PropertyPair& ae_prop,
    const char* match_name, const Converter& conv = {})
{
    if ( ae_prop.match_name != match_name )
        return false;

    if ( ae_prop.value->class_type() != PropertyBase::Property )
    {
        io->warning(AepFormat::tr("Expected property for %1").arg(match_name));
        return true;
    }

    if ( !load_static_property(prop, static_cast<const Property&>(*ae_prop.value), conv) )
        io->warning(AepFormat::tr("Could not find value for %1").arg(match_name));

    return true;
}

template<class T, class Converter=DefaultConverter<T>>
bool load_animated_property(
    model::AnimatedProperty<T>& prop, const Property& ae_prop, const Converter& conv = {}
)
{
    if ( !ae_prop.animated && ae_prop.value.type() )
    {
        prop.set(conv(ae_prop.value));
        return true;
    }

    for ( const auto& aekf : ae_prop.keyframes )
    {
        auto kf = prop.set_keyframe(aekf.time, conv(aekf.value));
        /// \todo easing
        /// \todo position bezier
        if ( aekf.transition_type == KeyframeTransitionType::Hold )
            kf->set_transition(model::KeyframeTransition(model::KeyframeTransition::Hold));
    }

    return true;
}

template<class T, class Converter=DefaultConverter<T>>
bool load_property(ImportExport* io, model::AnimatedProperty<T>& prop,
                   const PropertyPair& ae_prop, const char* match_name, const Converter& conv = {})
{
    if ( ae_prop.match_name != match_name )
        return false;

    if ( ae_prop.value->class_type() != PropertyBase::Property )
    {
        io->warning(AepFormat::tr("Expected property for %1").arg(match_name));
        return true;
    }

    if ( !load_animated_property(prop, static_cast<const Property&>(*ae_prop.value), conv) )
        io->warning(AepFormat::tr("Could convert %1").arg(match_name));

    return true;
}

bool convert_shape_reverse(const PropertyValue& v)
{
    return convert_value<int>(v) == 3;
}

qreal convert_percent(const PropertyValue& v)
{
    return convert_value<qreal>(v) / 100;
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
    if ( prop.match_name == "ADBE Vector Group" )
    {
        auto gp = std::make_unique<model::Group>(document);
        load_transform(gp->transform.get(), (*prop.value)["ADBE Vector Transform Group"], "Vector ");

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
        PROP(inner_roundness, "ADBE Vector Star Inner Roundess")
        PROP(outer_roundness, "ADBE Vector Star Outer Roundess")
    END
    OBJ("ADBE Vector Shape - Group", Path)
        PROP(reversed, "ADBE Vector Shape Direction", &convert_shape_reverse)
        PROP(shape, "ADBE Vector Shape")
    END
    OBJ("ADBE Vector Graphic - Fill", Fill)
        IGNORE("ADBE Vector Blend Mode")
        PROP(color, "ADBE Vector Fill Color")
        PROP(opacity, "ADBE Vector Fill Opacity", &convert_percent)
        PROP(fill_rule, "ADBE Vector Fill Rule", &convert_enum<model::Fill::Rule>)
        IGNORE("ADBE Vector Composite Order") /// \todo could be parsed
    END
    OBJ("ADBE Vector Graphic - Stroke", Stroke)
        IGNORE("ADBE Vector Blend Mode")
        PROP(color, "ADBE Vector Stroke Color")
        PROP(opacity, "ADBE Vector Stroke Opacity", &convert_percent)
        PROP(width, "ADBE Vector Stroke Width")
        PROP(cap, "ADBE Vector Stroke Line Cap", &convert_enum<model::Stroke::Cap>)
        PROP(join, "ADBE Vector Stroke Line Join", &convert_enum<model::Stroke::Join>)
        PROP(miter_limit, "ADBE Vector Stroke Miter Limit")
        IGNORE("ADBE Vector Stroke Dashes")
        IGNORE("ADBE Vector Stroke Taper")
        IGNORE("ADBE Vector Stroke Wave")
        IGNORE("ADBE Vector Composite Order") /// \todo could be parsed
    END
    else if ( prop.match_name == "ADBE Vector Graphic - G-Fill" ) {
        auto shape = std::make_unique<model::Fill>(document);
        auto grad_colors = document->assets()->gradient_colors->values.insert(
            std::make_unique<glaxnimate::model::GradientColors>(document)
        );
        auto grad = document->assets()->gradients->values.insert(
            std::make_unique<glaxnimate::model::Gradient>(document)
        );

        for ( const auto& p : *prop.value )
        {
            IGNORE("ADBE Vector Blend Mode")
            PROP(opacity, "ADBE Vector Fill Opacity", &convert_percent)
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
    else if ( prop.match_name == "ADBE Vector Graphic - G-Stroke" ) {
        auto shape = std::make_unique<model::Stroke>(document);
        auto grad_colors = document->assets()->gradient_colors->values.insert(
            std::make_unique<glaxnimate::model::GradientColors>(document)
        );
        auto grad = document->assets()->gradients->values.insert(
            std::make_unique<glaxnimate::model::Gradient>(document)
        );

        for ( const auto& p : *prop.value )
        {
            IGNORE("ADBE Vector Blend Mode")
            PROP(opacity, "ADBE Vector Stroke Opacity", &convert_percent)
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
    /// \todo More shapes
    else
    {
        info(AepFormat::tr("Unknown shape %1").arg(prop.match_name));
        return {};
    }
}

void glaxnimate::io::aep::AepLoader::asset_layer(model::Layer* layer, const glaxnimate::io::aep::Layer& ae_layer, glaxnimate::io::aep::AepLoader::CompData& data)
{
    /// \todo
}

void glaxnimate::io::aep::AepLoader::text_layer(model::Layer* layer, const glaxnimate::io::aep::Layer& ae_layer, glaxnimate::io::aep::AepLoader::CompData& data)
{
    /// \todo
}

void AepLoader::load_transform(model::Transform* tf, const PropertyBase& prop, const QString& infix)
{
    /// \todo
}
