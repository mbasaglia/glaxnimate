/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <unordered_map>
#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include <variant>
#include <optional>

#include <QColor>
#include <QPointF>
#include <QVector3D>
#include <QFileInfo>
#include <QGradientStops>

#include "model/animation/frame_time.hpp"
#include "math/math.hpp"

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
    QString match_name;
    std::unique_ptr<PropertyBase> value;
};

struct PropertyGroup : PropertyBase
{
    bool visible = true;
    bool split_position = false;
    QString name = "";
    std::vector<PropertyPair> properties;

    Type class_type() const noexcept override { return PropertyBase::PropertyGroup; }

    PropertyBase* property(const QString& match_name)
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
    double mid_point;
    Type value;
};

template<class T>
class GradientStops : public std::vector<GradientStop<T>>
{
public:
    using Stop = GradientStop<T>;
    using std::vector<Stop>::vector;

    int size() const
    {
        return std::vector<Stop>::size();
    }

    T value_at(double t, int& index) const
    {
        if ( this->empty() )
            return 1;

        if ( this->size() == 1 )
            return this->front().value;

        if ( t >= this->back().offset || index + 1 >= this->size() )
        {
            index = this->size();
            return this->back().value;
        }

        while ( t >= (*this)[index+1].offset )
            index++;

        if ( index + 1 >= this->size() )
            return this->back().value;

        const auto& before = (*this)[index];
        const auto& after = (*this)[index+1];
        auto factor = math::unlerp(before.offset, after.offset, t);

        if ( !qFuzzyCompare(before.mid_point, 0.5) )
        {
            auto vbefore = before.value;
            auto vafter = after.value;
            auto vmid = math::lerp(before.value, after.value, before.mid_point);
            if ( factor < after.mid_point )
            {
                factor = math::unlerp(0., before.mid_point, factor);
                vafter = vmid;
            }
            else
            {
                factor = math::unlerp(before.mid_point, 1., factor);
                vbefore = vmid;
            }
            return math::lerp(vbefore, vafter, factor);
        }

        return math::lerp(before.value, after.value, factor);
    }

    GradientStops split_midpoints() const
    {
        double midpoint = 0.5;
        Stop previous;
        GradientStops stops;
        for ( const auto& stop : *this )
        {
            if ( !qFuzzyCompare(midpoint, 0.5) )
            {
                auto midoffset = math::lerp(previous.offset, stop.offset, midpoint);
                auto midvalue = math::lerp(previous.value, stop.value, midpoint);
                stops.push_back({midoffset, 0.5, midvalue});
            }

            midpoint = stop.mid_point;
            stops.push_back({stop.offset, 0.5, stop.value});
        }

        return stops;
    }
};

struct Gradient
{
    GradientStops<double> alpha_stops;
    GradientStops<QColor> color_stops;

    QGradientStops to_qt() const
    {
        QGradientStops stops;
        int index = 0;
        for ( const auto& stop : color_stops )
        {
            auto alpha = alpha_stops.value_at(stop.offset, index);
            auto color = stop.value;
            color.setAlphaF(alpha);
            stops.push_back({stop.offset, color});
        }
        return stops;
    }
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
    QString name = "";
};

struct Font
{
    QString family;
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
    QString text;
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
public:
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

    template<class T>
    PropertyValue(T&& v) : value(std::forward<T>(v)) {}
    PropertyValue() = default;
    PropertyValue(PropertyValue& v) = delete;
    PropertyValue(const PropertyValue& v) = delete;
    PropertyValue(PropertyValue&& v) = default;
    PropertyValue& operator=(PropertyValue&& v) = default;

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
    std::vector<double> in_speed;
    std::vector<double> out_influence;
    std::vector<double> out_speed;
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
    std::optional<QString> expression;

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
    bool motion_blur = false;
    bool locked = false;
    bool shy = false;
    bool continuously_rasterize = false;
    Id asset_id = 0;
    LabelColors label_color = LabelColors::None;
    QString name = "";
    LayerType type = LayerType::ShapeLayer;
    Id parent_id = 0;
    TrackMatteType matte_mode = TrackMatteType::None;
    Id matte_id = 0;

    PropertyGroup properties;
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
    QString matchName = "";
    QString name = "";
    EffectParameterType type = EffectParameterType::Unknown;
    PropertyValue defaultValue;
    PropertyValue lastValue;
};

struct EffectDefinition
{
    QString matchName;
    QString name;
    std::vector<EffectParameter*> parameters;
    std::map<QString, EffectParameter> parameter_map;
};

struct EffectInstance : PropertyBase
{
    QString name;
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
    QString name = "";

    virtual ~FolderItem() noexcept = default;
    virtual Type type() const noexcept = 0;
};

struct Composition : FolderItem
{
    std::vector<std::unique_ptr<Layer>> layers;
    double time_scale = 0;
    model::FrameTime playhead_time = 0;
    model::FrameTime in_time = 0;
    model::FrameTime out_time = 0;
    model::FrameTime duration = 0;
    QColor color;
    double width = 0;
    double height = 0;
    double framerate = 0;
    std::unique_ptr<Layer> markers;
    std::vector<std::unique_ptr<Layer>> views;

    Type type() const noexcept override { return FolderItem::Composition; }

    model::FrameTime time_to_frames(model::FrameTime time) const
    {
        return time / time_scale;
    };
};


struct Asset : FolderItem
{
    int width = 0;
    int height = 0;

};

struct FileAsset : Asset
{
    QFileInfo path;

    Type type() const noexcept override { return FolderItem::Asset; }
};

struct Solid : Asset
{
    QColor color;

    Type type() const noexcept override { return FolderItem::Solid; }
};

struct Folder : FolderItem
{
    std::vector<std::unique_ptr<FolderItem>> items;

    template<class T>
    T* add()
    {
        auto item = std::make_unique<T>();
        auto ptr = item.get();
        items.push_back(std::move(item));
        return ptr;
    }

    Type type() const noexcept override { return FolderItem::Folder; }
};

struct Project
{
    std::unordered_map<Id, FolderItem*> assets;
    Folder folder;
    std::vector<Composition*> compositions;
    std::unordered_map<QString, EffectDefinition> effects;
    FolderItem* current_item = nullptr;
};

} // namespace glaxnimate::io::aep
