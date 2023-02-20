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
    {"DocumentNode", {
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
        FieldInfo("meta", Custom),
    }},
    // Layer is converted explicitly
    {"__Layer__", {
        FieldInfo{"op", Custom},
        FieldInfo{"ip", Custom},
        FieldInfo("ddd"),
        FieldInfo("hd", Custom),
        FieldInfo("ty", Custom),
        FieldInfo("parent", Custom),
        FieldInfo("sr"),
        FieldInfo("ks", Custom),
        FieldInfo("ao"),
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
        FieldInfo{"cix"},
        FieldInfo{"bm"},
        FieldInfo{"hd", Custom},
    }},
    {"Shape", {
        FieldInfo{"d", "reversed", EnumMap{{
            {1, 3},
            {0, 1},
        }}},
        FieldInfo{"closed", Custom}, // Old attribute
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
        FieldInfo{"is", "inner_roundness", FloatMult(100)},
        FieldInfo{"os", "outer_roundness", FloatMult(100)},
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
        FieldInfo{"c", Custom},
        FieldInfo{"fillEnabled", Custom},
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
        FieldInfo{"tm"},
    }},
    {"Repeater", {
        FieldInfo{"c", "copies"},
        FieldInfo{"o"},
        FieldInfo{"m"},
        FieldInfo{"tr", Custom},
    }},
    {"Trim", {
        FieldInfo{"s", "start", FloatMult(100)},
        FieldInfo{"e", "end", FloatMult(100)},
        FieldInfo{"o", "offset", FloatMult(360)},
        FieldInfo{"m", "multiple"},
    }},
    {"InflateDeflate", {
        FieldInfo{"a", "amount", FloatMult(100)},
    }},
    {"RoundCorners", {
        FieldInfo{"r", "radius"},
    }},
    {"OffsetPath", {
        FieldInfo{"a", "amount"},
        FieldInfo{"lj", "join"},
        FieldInfo{"ml", "miter_limit"},
    }},
    {"ZigZag", {
        FieldInfo{"s", "amplitude"},
        FieldInfo{"r", "frequency"},
        FieldInfo{"pt", "style"},
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
    // "gf" (Gradient Fill) and "gs" (Gradient Stroke) are handled by fill/stroke
    // "tr" is not a shape but a property of groups
    // "mm" (Merge), "tw" (Twist), are not supported by lottie
    {"Trim", "tm"},
    {"Repeater", "rp"},
    {"RoundCorners", "rd"},
    {"InflateDeflate", "pb"},
    {"OffsetPath", "op"},
    {"ZigZag", "zz"},
};

const QMap<QString, QString> shape_types_repeat = {
    {"gf", "Fill"},
    {"gs", "Stroke"},
};

const QMap<int, QString> unsupported_layers = {
    {6, "Audio"},
    {7, "Pholder Video"},
    {8, "Image Sequence"},
    {9, "Video"},
};

} // namespace glaxnimate::io::lottie::detail
