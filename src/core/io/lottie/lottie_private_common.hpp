/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "app/log/log.hpp"

#include "model/shapes/group.hpp"
#include "model/shapes/layer.hpp"
#include "model/shapes/precomp_layer.hpp"
#include "model/shapes/rect.hpp"
#include "model/shapes/ellipse.hpp"
#include "model/shapes/path.hpp"
#include "model/shapes/polystar.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/stroke.hpp"
#include "model/shapes/image.hpp"
#include "model/shapes/text.hpp"
#include "model/shapes/repeater.hpp"

#include "lottie_format.hpp"

namespace glaxnimate::io::lottie::detail {

class ValueTransform
{
public:
    virtual ~ValueTransform() {}

    virtual QVariant to_lottie(const QVariant& v, model::FrameTime) const = 0;
    virtual QVariant from_lottie(const QVariant& v, model::FrameTime) const = 0;
};

class FloatMult : public ValueTransform
{
public:
    explicit FloatMult(float factor) : factor(factor) {}

    QVariant to_lottie(const QVariant& v, model::FrameTime) const override
    {
        return v.toFloat() * factor;
    }

    QVariant from_lottie(const QVariant& v, model::FrameTime) const override
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

    QVariant to_lottie(const QVariant& v, model::FrameTime) const override
    {
        return values[v.toInt()];
    }

    QVariant from_lottie(const QVariant& v, model::FrameTime) const override
    {
        return values.key(v.toInt());
    }

    QMap<int, int> values;
};

class IntBool : public ValueTransform
{
public:
    QVariant to_lottie(const QVariant& v, model::FrameTime) const override
    {
        return int(v.toBool());
    }

    QVariant from_lottie(const QVariant& v, model::FrameTime) const override
    {
        return bool(v.toInt());
    }
};

class GradientLoad : public ValueTransform
{
public:
    GradientLoad(int count) : count(count) {}

    QVariant to_lottie(const QVariant&, model::FrameTime) const override { return {}; }

    QVariant from_lottie(const QVariant& v, model::FrameTime) const override
    {
        auto vlist = v.toList();
        if ( vlist.size() < count * 4 )
            return {};

        QGradientStops s;
        s.reserve(count);
        bool alpha = vlist.size() >= count * 6;

        for ( int i = 0; i < count; i++ )
        {
            s.push_back({
                vlist[i*4].toDouble(),
                QColor::fromRgbF(
                    vlist[i*4+1].toDouble(),
                    vlist[i*4+2].toDouble(),
                    vlist[i*4+3].toDouble(),
                    alpha ? vlist[count*4+2*i+1].toDouble() : 1
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

    QVariant to_lottie(const QVariant& v, model::FrameTime t) const
    {
        if ( !trans )
            return v;
        return trans->to_lottie(v, t);
    }

    QVariant from_lottie(const QVariant& v, model::FrameTime t) const
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
    Auto,
    AnimatedToStatic,
    Ignored,
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
        : name(QString::fromUtf8(name)), lottie(QString::fromUtf8(lottie)), essential(essential), mode(Auto), transform(std::move(transform))
    {}

    FieldInfo(const char* lottie, FieldMode mode = Ignored)
        : lottie(QString::fromUtf8(lottie)), essential(false), mode(mode)
    {}

    FieldInfo(const char* lottie, const char* name, FieldMode mode, bool essential = true)
        : name(QString::fromUtf8(name)), lottie(QString::fromUtf8(lottie)), essential(essential), mode(mode)
    {}
};

// static mapping data
const QMap<QString, QVector<FieldInfo>> fields = {
    {"DocumentNode"_qs, {
        FieldInfo{"nm", "name", {}, false},
        FieldInfo{"mn", "uuid", {}, false},
    }},
    {"Composition"_qs, {
        FieldInfo("layers", Custom),
        FieldInfo("id", Custom),
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
        FieldInfo("meta", Custom),
        FieldInfo("props"),
        FieldInfo("slots"),
    }},
    // Layer is converted explicitly
    {"__Layer__"_qs, {
        FieldInfo{"op", Custom},
        FieldInfo{"ip", Custom},
        FieldInfo("ddd"),
        FieldInfo("hd", Custom),
        FieldInfo("ty", Custom),
        FieldInfo("parent", Custom),
        FieldInfo("sr"),
        FieldInfo("ks", Custom),
        FieldInfo("ao", "auto_orient", IntBool()),
        FieldInfo{"st", Custom},
        FieldInfo("bm"),
        FieldInfo("tt"),
        FieldInfo("td"),
        FieldInfo("ind", Custom),
        FieldInfo("cl"),
        FieldInfo("ln"),
        FieldInfo("hasMasks", Custom),
        FieldInfo("masksProperties", Custom),
        FieldInfo("ef"),
        FieldInfo("bounds"), // old, no longer there
        FieldInfo("ct"),
    }},
    {"Transform"_qs, {
        FieldInfo{"a", "anchor_point"},
        FieldInfo("px", Custom),
        FieldInfo("py", Custom),
        FieldInfo("pz", Custom),
        FieldInfo{"p", "position"},
        FieldInfo{"s", "scale"},
        FieldInfo{"r", "rotation"},
        FieldInfo("rx", Custom),
        FieldInfo("ry", Custom),
        FieldInfo("rz", Custom),
        FieldInfo("o"),
        FieldInfo("sk"),
        FieldInfo("sa"),
        FieldInfo("nm"),
    }},
    {"ShapeElement"_qs, {
        FieldInfo{"ty", Custom},
        FieldInfo{"ix"},
        FieldInfo{"cix"},
        FieldInfo{"bm"},
        FieldInfo{"hd", Custom},
    }},
    {"Shape"_qs, {
        FieldInfo{"d", "reversed", EnumMap{{
            {1, 3},
            {0, 1},
        }}},
        FieldInfo{"closed", Custom}, // Old attribute
    }},
    {"Rect"_qs, {
        FieldInfo{"p", "position"},
        FieldInfo{"s", "size"},
        FieldInfo{"r", "rounded"},
    }},
    {"Ellipse"_qs, {
        FieldInfo{"p", "position"},
        FieldInfo{"s", "size"},
    }},
    {"Path"_qs, {
        FieldInfo{"ks", "shape"},
        FieldInfo{"ind"},
    }},
    {"PolyStar"_qs, {
        FieldInfo{"p", "position"},
        FieldInfo{"or", "outer_radius"},
        FieldInfo{"ir", "inner_radius"},
        FieldInfo{"is", "inner_roundness", FloatMult(100)},
        FieldInfo{"os", "outer_roundness", FloatMult(100)},
        FieldInfo{"r", "angle"},
        FieldInfo{"pt", "points"},
        FieldInfo{"sy", "type"},
    }},
    {"Group"_qs, {
        FieldInfo{"np"},
        FieldInfo{"it", Custom},
    }},
    {"Styler"_qs, {
        FieldInfo{"o", "opacity", FloatMult(100)},
        FieldInfo{"c", Custom},
        FieldInfo{"fillEnabled", Custom},
    }},
    {"Fill"_qs, {
        FieldInfo{"r", "fill_rule", EnumMap{{
            {model::Fill::NonZero, 1},
            {model::Fill::EvenOdd, 2},
        }}},
    }},
    {"Stroke"_qs, {
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
    {"Bitmap"_qs, {
        FieldInfo{"h", "height"},
        FieldInfo{"w", "width"},
        FieldInfo{"id", Custom},
        FieldInfo{"p", Custom},
        FieldInfo{"u", Custom},
        FieldInfo{"e", Custom},
    }},
    {"Gradient"_qs, {
        FieldInfo{"s", "start_point"},
        FieldInfo{"e", "end_point"},
        FieldInfo{"t", "type"},
        FieldInfo{"h", Custom}, /// \todo
        FieldInfo{"a", Custom}, /// \todo
        FieldInfo{"g", Custom},
    }},
    {"PreCompLayer"_qs, {
        FieldInfo{"refId", Custom},
        FieldInfo{"w", Custom},
        FieldInfo{"h", Custom},
        FieldInfo{"tm"},
    }},
    {"Repeater"_qs, {
        FieldInfo{"c", "copies"},
        FieldInfo{"o"},
        FieldInfo{"m"},
        FieldInfo{"tr", Custom},
    }},
    {"Trim"_qs, {
        FieldInfo{"s", "start", FloatMult(100)},
        FieldInfo{"e", "end", FloatMult(100)},
        FieldInfo{"o", "offset", FloatMult(360)},
        FieldInfo{"m", "multiple"},
    }},
    {"InflateDeflate"_qs, {
        FieldInfo{"a", "amount", FloatMult(100)},
    }},
    {"RoundCorners"_qs, {
        FieldInfo{"r", "radius"},
    }},
    {"OffsetPath"_qs, {
        FieldInfo{"a", "amount"},
        FieldInfo{"lj", "join", EnumMap{{
            {model::Stroke::MiterJoin, 1},
            {model::Stroke::RoundJoin, 2},
            {model::Stroke::BevelJoin, 3},
        }}},
        FieldInfo{"ml", "miter_limit"},
    }},
    {"ZigZag"_qs, {
        FieldInfo{"s", "amplitude"},
        FieldInfo{"r", "frequency"},
        FieldInfo{"pt", "style", AnimatedToStatic},
    }},
};
const QMap<QString, QString> shape_types = {
    {"Rect"_qs, "rc"_qs},
    {"PolyStar"_qs, "sr"_qs},
    {"Ellipse"_qs, "el"_qs},
    {"Path"_qs, "sh"_qs},
    {"Group"_qs, "gr"_qs},
    {"Layer"_qs, "gr"_qs},
    {"Fill"_qs, "fl"_qs},
    {"Stroke"_qs, "st"_qs},
    // "gf" (Gradient Fill) and "gs" (Gradient Stroke) are handled by fill/stroke
    // "tr" is not a shape but a property of groups
    // "mm" (Merge), "tw" (Twist), are not supported by lottie
    {"Trim"_qs, "tm"_qs},
    {"Repeater"_qs, "rp"_qs},
    {"RoundCorners"_qs, "rd"_qs},
    {"InflateDeflate"_qs, "pb"_qs},
    {"OffsetPath"_qs, "op"_qs},
    {"ZigZag"_qs, "zz"_qs},
};

const QMap<QString, QString> shape_types_repeat = {
    {"gf"_qs, "Fill"_qs},
    {"gs"_qs, "Stroke"_qs},
};

const QMap<int, QString> unsupported_layers = {
    {6, "Audio"_qs},
    {7, "Pholder Video"_qs},
    {8, "Image Sequence"_qs},
    {9, "Video"_qs},
};

} // namespace glaxnimate::io::lottie::detail
