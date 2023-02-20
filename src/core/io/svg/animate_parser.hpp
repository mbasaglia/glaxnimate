/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <variant>

#include <QDomElement>
#include <QRegularExpression>

#include "model/animation/keyframe_transition.hpp"
#include "model/animation/frame_time.hpp"
#include "model/animation/join_animatables.hpp"
#include "math/bezier/bezier.hpp"
#include "app/utils/string_view.hpp"

#include "path_parser.hpp"
#include "detail.hpp"
#include "io/animated_properties.hpp"

namespace glaxnimate::io::svg {

QColor parse_color(const QString& string);

} // namespace glaxnimate::io::svg

namespace glaxnimate::io::svg::detail {

using namespace glaxnimate::io::detail;

class AnimateParser
{
public:
    struct AnimatedProperties : public glaxnimate::io::detail::AnimatedProperties
    {
        QDomElement element;

        bool prepare_joined(std::vector<JoinedProperty>& props) const override
        {
            for ( auto& p : props )
            {
                if ( p.prop.index() == 1 )
                {
                    if ( !element.hasAttribute(*p.get<1>()) )
                        return false;
                    p.prop = split_values(element.attribute(*p.get<1>()));
                }
            }

            return true;
        }
    };

    static std::vector<qreal> split_values(const QString& v)
    {
        if ( !v.contains(separator) )
        {
            bool ok = false;
            qreal val = v.toDouble(&ok);
            if ( ok )
                return {val};

            QColor c(v);
            if ( c.isValid() )
                return {c.redF(), c.greenF(), c.blueF(), c.alphaF()};
            return {};
        }

        auto split = ::utils::split_ref(v, separator,
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        Qt::SkipEmptyParts
#else
        QString::SkipEmptyParts
#endif
        );
        std::vector<qreal> values;
        values.reserve(split.size());
        for ( const auto& r : split )
            values.push_back(r.toDouble());
        return values;
    }

    model::FrameTime clock_to_frame(const QString& clock)
    {
        auto match = clock_re.match(clock, 0, QRegularExpression::PartialPreferCompleteMatch);
        if ( !match.hasMatch() )
            return 0;

        static constexpr const qreal minutes = 60;
        static constexpr const qreal hours = 60*60;
        static const std::map<QString, qreal> units = {
            {"ms", 0.001},
            {"s", 1.0},
            {"min", minutes},
            {"h", hours}
        };

        if ( !match.captured("unit").isEmpty() )
            return match.captured("timecount").toDouble() * units.at(match.captured("unit")) * fps;

        return (
            match.captured("hours").toDouble() * hours +
            match.captured("minutes").toDouble() * minutes +
            match.captured("seconds").toDouble()
        ) * fps;
    }

    ValueVariant parse_value(const QString& str, ValueVariant::Type type) const
    {
        switch ( type )
        {
            case ValueVariant::Vector:
                return split_values(str);
            case ValueVariant::Bezier:
                return PathDParser(str).parse();
            case ValueVariant::String:
                return str;
            case ValueVariant::Color:
                return parse_color(str);
        }

        return {};
    }

    std::vector<ValueVariant> get_values(const QDomElement& animate)
    {
        QString attr = animate.attribute("attributeName");
        ValueVariant::Type type = ValueVariant::Vector;
        if ( attr == "d" )
            type = ValueVariant::Bezier;
        else if ( attr == "display" )
            type = ValueVariant::String;
        else if ( attr == "fill" || attr == "stroke" || attr == "stop-color" )
            type = ValueVariant::Color;

        std::vector<ValueVariant> values;

        if ( animate.hasAttribute("values") )
        {
            auto val_str = animate.attribute("values").split(frame_separator_re,
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        Qt::SkipEmptyParts
#else
        QString::SkipEmptyParts
#endif
            );
            values.reserve(val_str.size());
            for ( const auto& val : val_str )
                values.push_back(parse_value(val, type));

            if ( values.size() < 2 )
            {
                warning("Not enough values in `animate`");
                return {};
            }

            for ( uint i = 1; i < values.size(); i++ )
            {
                if ( !values[i].compatible(values[0]) )
                {
                    warning("Mismatching `values` in `animate`");
                    return {};
                }
            }
        }
        else
        {
            if ( animate.hasAttribute("from") )
            {
                values.push_back(parse_value(animate.attribute("from"), type));
            }
            else
            {
                if ( attr == "transform" )
                {
                    warning("You need to set `values` or `from` in `animateTransform`");
                    return {};
                }
                else if ( !animate.hasAttribute(attr) )
                {
                    warning("Missing `from` in `animate`");
                    return {};
                }

                values.push_back(parse_value(animate.attribute(attr), type));
            }

            if ( animate.hasAttribute("to") )
            {
                values.push_back(parse_value(animate.attribute("to"), type));
            }
            else if ( type == ValueVariant::Vector && animate.hasAttribute("by") )
            {
                auto by = split_values(animate.attribute("to"));
                if ( by.size() != values[0].vector().size() )
                {
                    warning("Mismatching `by` and `from` in `animate`");
                    return {};
                }

                for ( uint i = 0; i < by.size(); i++ )
                    by[i] += values[0].vector()[i];

                values.push_back(std::move(by));
            }
            else
            {
                warning("Missing `to` or `by` in `animate`");
                return {};
            }
        }

        if ( type == ValueVariant::Bezier )
        {
            for ( const auto& v : values )
            {
                if ( v.bezier().size() != 1 )
                {
                    warning("Can only load animated `d` if each keyframe has exactly 1 path");
                    return {};
                }
            }
        }

        return values;
    }

    void parse_animate(const QDomElement& animate, AnimatedProperty& prop)
    {
        if ( !prop.keyframes.empty() )
        {
            warning("Multiple `animate` for the same property");
            return;
        }

        model::FrameTime start_time = 0;
        model::FrameTime end_time = 0;

        if ( animate.hasAttribute("begin") )
            start_time = clock_to_frame(animate.attribute("begin"));

        if ( animate.hasAttribute("dur") )
            end_time = start_time + clock_to_frame(animate.attribute("dur"));
        else if ( animate.hasAttribute("end") )
            end_time = clock_to_frame(animate.attribute("end"));

        if ( start_time >= end_time )
        {
            warning("Invalid timings in `animate`");
            return;
        }

        std::vector<ValueVariant> values = get_values(animate);
        if ( values.empty() )
            return;

        std::vector<model::FrameTime> times;
        times.reserve(values.size());

        if ( animate.hasAttribute("keyTimes") )
        {
            auto strings = animate.attribute("keyTimes").split(frame_separator_re,
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        Qt::SkipEmptyParts
#else
        QString::SkipEmptyParts
#endif
            );
            if ( strings.size() != int(values.size()) )
            {
                warning("`keyTimes` and `values` mismatch");
                return;
            }

            for ( const auto& s : strings )
                times.push_back(math::lerp(start_time, end_time, s.toDouble()));
        }
        else
        {
            for ( qreal i = 0; i < values.size(); i++ )
                times.push_back(math::lerp(start_time, end_time, i/(values.size()-1)));
        }

        std::vector<model::KeyframeTransition> transitions;
        QString calc = animate.attribute("calcMode", "linear");
        if ( calc == "spline" )
        {
            if ( !animate.hasAttribute("keySplines") )
            {
                warning("Missing `keySplines`");
                return;
            }

            auto splines = animate.attribute("keySplines").split(frame_separator_re,
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        Qt::SkipEmptyParts
#else
        QString::SkipEmptyParts
#endif
            );
            if ( splines.size() != int(values.size()) - 1 )
            {
                warning("Wrong number of `keySplines` values");
                return;
            }

            transitions.reserve(values.size());
            for ( const auto& spline : splines )
            {
                auto params = split_values(spline);
                if ( params.size() != 4 )
                {
                    warning("Invalid value for `keySplines`");
                    return;
                }
                transitions.push_back({{params[0], params[1]}, {params[2], params[3]}});
            }
            transitions.emplace_back();
        }
        else
        {
            model::KeyframeTransition def;
            if ( calc == "discrete" )
                def.set_hold(true);
            transitions = std::vector<model::KeyframeTransition>(values.size(), def);
        }

        prop.keyframes.reserve(values.size());

        for ( uint i = 0; i < values.size(); i++ )
            prop.keyframes.push_back({times[i], values[i], transitions[i]});
    }

    AnimatedProperties parse_animated_properties(const QDomElement& parent)
    {
        AnimatedProperties props;
        props.element = parent;

        for ( const auto& domnode : ItemCountRange(parent.childNodes()) )
        {
            if ( domnode.isElement() )
            {
                auto child = domnode.toElement();

                if ( child.tagName() == "animate" )
                {
                    if ( child.hasAttribute("attributeName") )
                        parse_animate(child, props.properties[child.attribute("attributeName")]);
                }
            }
        }

        return props;
    }

    AnimatedProperties parse_animated_transform(const QDomElement& parent)
    {
        AnimatedProperties props;
        props.element = parent;

        for ( const auto& domnode : ItemCountRange(parent.childNodes()) )
        {
            if ( domnode.isElement() )
            {
                auto child = domnode.toElement();
                if ( child.tagName() == "animateTransform" )
                {
                    if ( child.hasAttribute("type") && child.attribute("attributeName") == "transform" )
                        parse_animate(child, props.properties[child.attribute("type")]);
                }
            }
        }

        return props;
    }

    void warning(const QString& msg)
    {
        if ( on_warning )
            on_warning(msg);
    }

    qreal fps = 60;
    std::function<void(const QString&)> on_warning;
    static const QRegularExpression separator;
    static const QRegularExpression clock_re;
    static const QRegularExpression frame_separator_re;
};

} // namespace glaxnimate::io::svg::detail
