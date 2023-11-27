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
#include "math/vector.hpp"
#include "utils/iterator.hpp"

namespace glaxnimate::io::aep {

using Id = std::uint32_t;

struct PropertyIterator;

struct PropertyPair;

struct PropertyBase
{
    enum Type { Null, PropertyGroup, Property, TextProperty, EffectInstance, Mask };
    virtual ~PropertyBase() noexcept = default;
    virtual Type class_type() const noexcept { return Null; }

    const PropertyBase* get(const QString& key) const;
    virtual const PropertyPair* get_pair(const QString& key) const { Q_UNUSED(key); return nullptr; }

    explicit operator bool() const
    {
        return class_type() != Null;
    }

    const PropertyBase& operator[](const QString& key) const
    {
        auto prop = get(key);
        if ( prop )
            return *prop;

        static PropertyBase null_property;
        return null_property;
    }
    virtual PropertyIterator begin() const;
    virtual PropertyIterator end() const;
};

struct PropertyPair
{
    QString match_name;
    std::unique_ptr<PropertyBase> value;
};

inline const PropertyBase* PropertyBase::get(const QString& key) const
{
    if ( auto p = get_pair(key) )
        return p->value.get();
    return nullptr;
}

struct PropertyIterator : public utils::RandomAccessIteratorWrapper<PropertyIterator, std::vector<PropertyPair>::const_iterator>
{
public:
    PropertyIterator(InternalIterator it = {}) : Parent(it) {}
    const PropertyPair* operator->() const { return iter.operator->(); }
    const PropertyPair& operator*() const { return *iter; }
};

inline PropertyIterator PropertyBase::begin() const { return {}; }
inline PropertyIterator PropertyBase::end() const { return {}; }

struct PropertyGroup : PropertyBase
{
    bool visible = true;
    QString name = {};
    std::vector<PropertyPair> properties;

    Type class_type() const noexcept override { return PropertyBase::PropertyGroup; }

    const PropertyPair* property_pair(const QString& match_name) const
    {
        for ( const auto& prop : properties )
        {
            if ( prop.match_name == match_name )
                return &prop;
        }
        return nullptr;
    }

    const PropertyBase* property(const QString& match_name) const
    {
        if ( auto p = property_pair(match_name) )
            return p->value.get();
        return nullptr;
    }

    const PropertyPair* get_pair(const QString& key) const override
    {
        return property_pair(key);
    }

    PropertyIterator begin() const override
    {
        return properties.begin();
    }

    PropertyIterator end() const override
    {
        return properties.end();
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

    /**
     * \brief Converts points from the weird bounding box notation to absolute
     */
    QPointF convert_point(const QPointF& p) const
    {
        return {
            math::lerp(minimum.x(), maximum.x(), p.x()),
            math::lerp(minimum.y(), maximum.y(), p.y()),
        };
    }

    QPointF converted_point(int index) const
    {
        return convert_point(points[index]);
    }
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
            previous = stop;
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
        for ( const auto& stop : color_stops.split_midpoints() )
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
    QString name = {};
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

enum class TextVerticalAlign
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
    TextVerticalAlign vertical_align = TextVerticalAlign::Normal;
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
        std::nullptr_t, QPointF, QVector3D, QColor, qreal, aep::Gradient,
        aep::BezierData, aep::Marker, aep::TextDocument, aep::LayerSelection
    > value = nullptr;

    qreal magnitude() const
    {
        switch ( type() )
        {
            default:
            case None:
                return 0;
            case Vector2D:
                return math::length(std::get<QPointF>(value));
            case Vector3D:
                return math::length(std::get<QVector3D>(value));
            case Color:
            {
                const auto& c = std::get<QColor>(value);
                return math::hypot(c.red(), c.green(), c.blue(), c.alpha());
            }
            case Number:
                return std::get<qreal>(value);
        }
    }
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
    bool split = false;
    bool animated = false;
    bool is_component = false;
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
    qreal time_stretch = 1;
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
    QString name = {};
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
    QString match_name = {};
    QString name = {};
    EffectParameterType type = EffectParameterType::Unknown;
    PropertyValue default_value;
    PropertyValue last_value;
};

struct EffectDefinition
{
    QString match_name;
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


    const PropertyPair* get_pair(const QString& key) const override
    {
        return properties.property_pair(key);
    }
};

struct FolderItem
{
    enum Type { Composition, Folder, Asset, Solid };
    Id id;
    QString name = {};
    LabelColors label_color = LabelColors::None;

    virtual ~FolderItem() noexcept = default;
    virtual Type type() const noexcept = 0;
};

struct Composition : FolderItem
{
    std::vector<std::unique_ptr<Layer>> layers;
    std::uint16_t resolution_x = 0;
    std::uint16_t resolution_y = 0;
    double time_scale = 0;
    model::FrameTime playhead_time = 0;
    model::FrameTime in_time = 0;
    model::FrameTime out_time = 0;
    model::FrameTime duration = 0;
    QColor color;
    bool shy = false;
    bool motion_blur = false;
    bool frame_blending = false;
    bool preserve_framerate = false;
    bool preserve_resolution = false;
    double width = 0;
    double height = 0;
    std::uint32_t pixel_ratio_width = 1;
    std::uint32_t pixel_ratio_height = 1;
    double framerate = 0;
    std::uint16_t shutter_angle = 0;
    std::int32_t shutter_phase = 0;
    std::uint32_t samples_limit = 0;
    std::uint32_t samples_per_frame = 0;
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
