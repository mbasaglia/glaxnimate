#pragma once

#include <unordered_map>
#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include <variant>

#include <QColor>
#include <QPointF>
#include <QVector3D>

#include "model/animation/frame_time.hpp"

namespace glaxnimate::io::aep {

using Id = std::uint32_t;

struct PropertyBase
{
    enum Type { PropertyGroup, Property, TextProperty, EffectInstance, Mask };
    virtual ~PropertyBase() noexcept = default;
    virtual Type class_type() const noexcept = 0;
};

struct PropertyPair
{
    std::string match_name;
    std::unique_ptr<PropertyBase> value;
};

struct PropertyGroup : PropertyBase
{
    bool visible = true;
    bool splitPosition = false;
    std::string name = "";
    std::vector<PropertyPair> properties;

    Type class_type() const noexcept override { return PropertyBase::PropertyGroup; }

    PropertyBase* property(const std::string& match_name)
    {
        for ( const auto& prop : properties )
        {
            if ( prop.match_name == match_name )
                return prop.value.get();
        }
        return nullptr;
    }
};

enum class KeyframeTransitionType
{
    Linear = 1,
    Bezier = 2,
    Hold = 3,
};

enum class KeyframeBezierMode
{
    Normal,
    Continuous,
    Auto,
};


struct BezierData
{
    bool closed = false;
    QPointF minimum;
    QPointF maximum;
    std::vector<QPointF> points;
};

template<class Type>
struct GradientStop
{
    double offset;
    double midPoint;
    Type value;
};


struct Gradient
{
    GradientStop<double> alpha_stops;
    GradientStop<QColor> color_stops;
};

enum class LabelColors
{
    None,
    Red,
    Yellow,
    Aqua,
    Pink,
    Lavender,
    Peach,
    SeaFoam,
    Blue,
    Green,
    Purple,
    Orange,
    Brown,
    Fuchsia,
    Cyan,
    Sandstone,
    DarkGreen,
};

struct Marker
{
    model::FrameTime duration = 0;
    LabelColors label_color = LabelColors::None;
    bool is_protected = false;
    std::string name = "";
};

struct Font
{
    std::string family;
};

enum class TextTransform
{
    Normal,
    SmallCaps,
    AllCaps
};

enum class TextVericalAlign
{
    Normal,
    Superscript,
    Subscript,
};

enum class TextJustify
{
  Left,
  Right,
  Center,
  JustifyLastLineLeft,
  JustifyLastLineRight,
  JustifyLastLineCenter,
  JustifyLastLineFull,
};

struct LineStyle
{
    TextJustify text_justify = TextJustify::Left;
    // Number of characters the style applies to
    // Note that in AE LineStyle represents a line
    // Rather than an arbitrary span of text
    int character_count = 0;
};

struct CharacterStyle
{
    int font_index = 0;
    double size = 0;
    bool faux_bold = false;
    bool faux_italic = false;
    TextTransform text_transform = TextTransform::Normal;
    TextVericalAlign vertical_align = TextVericalAlign::Normal;
    QColor fill_color;
    QColor stroke_color;
    bool stroke_enabled = false;
    bool stroke_over_fill = false;
    double stroke_width = 0;
    // Number of characters the style applies to
    int character_count = 0;
};

struct TextDocument
{
    std::string text;
    std::vector<LineStyle> line_styles;
    std::vector<CharacterStyle> character_styles;
};

enum class LayerSource
{
    Layer = 0,
    Effects = -1,
    Masks = -2
};

struct LayerSelection
{
    Id layer_id;
    LayerSource layer_source = LayerSource::Layer;
};

class PropertyValue
{
    enum Index
    {
        None,
        Vector2D,
        Vector3D,
        Color,
        Number,
        Gradient,
        BezierData,
        Marker,
        TextDocument,
        LayerSelection
    };

    Index type() const { return Index(value.index()); }

    std::variant<
        std::nullptr_t, QPointF, QVector3D, QColor,  double, aep::Gradient,
        aep::BezierData, aep::Marker, aep::TextDocument, aep::LayerSelection
    > value = nullptr;
};

struct Keyframe
{
    PropertyValue value;
    model::FrameTime time = 0;
    std::vector<double> in_influence;
    std::vector<double> inSpeed;
    std::vector<double> outInfluence;
    std::vector<double> outSpeed;
    QPointF in_tangent;
    QPointF out_tangent;
    KeyframeTransitionType transition_type = KeyframeTransitionType::Linear;
    KeyframeBezierMode bezier_mode = KeyframeBezierMode::Normal;
    bool roving = false;
    LabelColors label_color = LabelColors::None;
};

enum class PropertyType
{
    Color,
    NoValue,
    Position,
    MultiDimensional,
    LayerSelection,
    Integer,
    MaskIndex,
};

struct Property : PropertyBase
{
    bool animated = false;
    int components = 0;
    PropertyValue value;
    std::vector<Keyframe> keyframes;
    PropertyType type = PropertyType::MultiDimensional;
    std::optional<std::string> expression;

    Type class_type() const noexcept override { return PropertyBase::Property; }
};

struct TextProperty : PropertyBase
{
    std::vector<Font> fonts;
    aep::Property documents;

    Type class_type() const noexcept override { return PropertyBase::TextProperty; }
};

enum class LayerQuality
{
    Wireframe,
    Draft,
    Best
};

enum class LayerType
{
    AssetLayer,
    LightLayer,
    CameraLayer,
    TextLayer,
    ShapeLayer
};


enum TrackMatteType
{
    None,
    Alpha,
    AlphaInverted,
    Luma,
    LumaInverted,
};

struct Layer
{
    Id id = 0;
    LayerQuality quality = LayerQuality::Draft;
    model::FrameTime start_time = 0;
    model::FrameTime in_time = 0;
    model::FrameTime out_time = 0;
    bool is_guide = false;
    bool bicubic_sampling = false;
    bool auto_orient = false;
    bool is_adjustment = false;
    bool threedimensional = false;
    bool solo = false;
    bool is_null = false;
    bool visible = true;
    bool effects_enabled = false;
    bool motion_blurenabled = false;
    bool locked = false;
    bool shy = false;
    bool continuously_rasterize = false;
    Id asset_id = 0;
    LabelColors label_color = LabelColors::None;
    std::string name = "";
    LayerType type = LayerType::ShapeLayer;
    Id parent_id = 0;
    PropertyGroup properties;
    TrackMatteType matte_mode = TrackMatteType::None;
    Id matte_id = 0;
};

enum class EffectParameterType
{
    Layer = 0,
    Scalar = 2,
    Angle = 3,
    Boolean = 4,
    Color = 5,
    Vector2D = 6,
    Enum = 7,
    Group = 9,
    Slider = 10,
    Unknown = 15,
    Vector3D = 16,
};

struct EffectParameter
{
    std::string matchName = "";
    std::string name = "";
    EffectParameterType type = EffectParameterType::Unknown;
    PropertyValue defaultValue;
    PropertyValue lastValue;
};

struct EffectDefinition
{
    std::string matchName;
    std::string name;
    std::vector<EffectParameter*> parameters;
    std::map<std::string, EffectParameter> parameter_map;
};

struct EffectInstance : PropertyBase
{
    std::string name;
    aep::PropertyGroup parameters;

    Type class_type() const noexcept override { return PropertyBase::EffectInstance; }
};

enum class MaskMode
{
    None,
    Add,
    Subtract,
    Intersect,
    Darken,
    Lighten,
    Difference
};

struct Mask : PropertyBase
{
    bool inverted = false;
    bool  locked = false;
    MaskMode mode = MaskMode::Add;
    aep::PropertyGroup properties;

    Type class_type() const noexcept override { return PropertyBase::Mask; }
};

struct FolderItem
{
    enum Type { Composition, Folder, Asset, Solid };
    Id id;
    std::string name = "";

    virtual ~FolderItem() noexcept = default;
    virtual Type type() const noexcept = 0;
};

struct Composition : FolderItem
{
    std::vector<Layer> layers;
    double time_scale = 0;
    model::FrameTime playhead_time = 0;
    model::FrameTime in_time = 0;
    model::FrameTime out_time = 0;
    model::FrameTime duration = 0;
    QColor color;
    double width = 0;
    double height = 0;
    double framerate = 0;
    Layer markers;
    std::vector<Layer> views;

    Type type() const noexcept override { return FolderItem::Composition; }
    model::FrameTime timeToFrame(model::FrameTime time) const
    {
        return time / time_scale;
    };
};

struct Asset : FolderItem
{
    std::string full_path;
    int width = 0;
    int height = 0;

    Type type() const noexcept override { return FolderItem::Asset; }
};

struct Solid : FolderItem
{
    QColor color;
    int width = 0;
    int height = 0;

    Type type() const noexcept override { return FolderItem::Solid; }
};

struct Folder : FolderItem
{
    std::vector<std::unique_ptr<FolderItem>> items;

    Type type() const noexcept override { return FolderItem::Folder; }
};

struct Project
{
    std::unordered_map<Id, FolderItem*> assets;
    Folder folder;
    std::vector<Composition*> compositions;
    std::unordered_map<std::string, EffectDefinition> effects;
    FolderItem* current_item = nullptr;
};

} // namespace glaxnimate::io::aep
