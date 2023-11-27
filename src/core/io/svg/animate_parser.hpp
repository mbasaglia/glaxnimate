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

        bool apply_motion(model::AnimatedProperty<QPointF>& prop, const QPointF& delta_pos = {}, model::Property<bool>* auto_orient = nullptr) const
        {
            auto motion = properties.find("motion"_qs);
            if ( motion == properties.end() )
                return false;

            if ( auto_orient )
                auto_orient->set(motion->second.auto_orient);

            for ( const auto& kf : motion->second.keyframes )
                prop.set_keyframe(kf.time, QPointF())->set_transition(kf.transition);

            if ( qFuzzyIsNull(math::length(delta_pos)) )
            {
                prop.set_bezier(motion->second.motion);
            }
            else
            {
                math::bezier::Bezier bez = motion->second.motion;
                for ( auto& p : bez )
                    p.translate(delta_pos);

                prop.set_bezier(bez);
            }

            return true;
        }

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
            {"ms"_qs, 0.001},
            {"s"_qs, 1.0},
            {"min"_qs, minutes},
            {"h"_qs, hours}
        };

        if ( !match.captured("unit"_qs).isEmpty() )
            return match.captured("timecount"_qs).toDouble() * units.at(match.captured("unit"_qs)) * fps;

        return (
            match.captured("hours"_qs).toDouble() * hours +
            match.captured("minutes"_qs).toDouble() * minutes +
            match.captured("seconds"_qs).toDouble()
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
        QString attr = animate.attribute("attributeName"_qs);
        ValueVariant::Type type = ValueVariant::Vector;
        if ( attr == "d"_qs )
            type = ValueVariant::Bezier;
        else if ( attr == "display"_qs )
            type = ValueVariant::String;
        else if ( attr == "fill"_qs || attr == "stroke"_qs || attr == "stop-color"_qs )
            type = ValueVariant::Color;

        std::vector<ValueVariant> values;

        if ( animate.hasAttribute("values"_qs) )
        {
            auto val_str = animate.attribute("values"_qs).split(frame_separator_re,
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
                warning("Not enough values in `animate`"_qs);
                return {};
            }

            for ( uint i = 1; i < values.size(); i++ )
            {
                if ( !values[i].compatible(values[0]) )
                {
                    warning("Mismatching `values` in `animate`"_qs);
                    return {};
                }
            }
        }
        else
        {
            if ( animate.hasAttribute("from"_qs) )
            {
                values.push_back(parse_value(animate.attribute("from"_qs), type));
            }
            else
            {
                if ( attr == "transform"_qs )
                {
                    warning("You need to set `values` or `from` in `animateTransform`"_qs);
                    return {};
                }
                else if ( !animate.hasAttribute(attr) )
                {
                    warning("Missing `from` in `animate`"_qs);
                    return {};
                }

                values.push_back(parse_value(animate.attribute(attr), type));
            }

            if ( animate.hasAttribute("to"_qs) )
            {
                values.push_back(parse_value(animate.attribute("to"_qs), type));
            }
            else if ( type == ValueVariant::Vector && animate.hasAttribute("by"_qs) )
            {
                auto by = split_values(animate.attribute("to"_qs));
                if ( by.size() != values[0].vector().size() )
                {
                    warning("Mismatching `by` and `from` in `animate`"_qs);
                    return {};
                }

                for ( uint i = 0; i < by.size(); i++ )
                    by[i] += values[0].vector()[i];

                values.push_back(std::move(by));
            }
            else
            {
                warning("Missing `to` or `by` in `animate`"_qs);
                return {};
            }
        }

        if ( type == ValueVariant::Bezier )
        {
            for ( const auto& v : values )
            {
                if ( v.bezier().size() != 1 )
                {
                    warning("Can only load animated `d` if each keyframe has exactly 1 path"_qs);
                    return {};
                }
            }
        }

        return values;
    }

    void register_time_range(model::FrameTime start_time, model::FrameTime end_time)
    {
        if ( !kf_range_initialized )
        {
            kf_range_initialized = true;
            min_kf = start_time;
            max_kf = end_time;
        }
        else
        {
            if ( min_kf > start_time )
                min_kf = start_time;
            if ( max_kf < end_time )
                max_kf = end_time;
        }
    }

    void parse_animate(const QDomElement& animate, AnimatedProperty& prop, bool motion)
    {
        if ( !prop.keyframes.empty() )
        {
            warning("Multiple `animate` for the same property"_qs);
            return;
        }

        model::FrameTime start_time = 0;
        model::FrameTime end_time = 0;

        if ( animate.hasAttribute("begin"_qs) )
            start_time = clock_to_frame(animate.attribute("begin"_qs));

        if ( animate.hasAttribute("dur"_qs) )
            end_time = start_time + clock_to_frame(animate.attribute("dur"_qs));
        else if ( animate.hasAttribute("end"_qs) )
            end_time = clock_to_frame(animate.attribute("end"_qs));

        if ( start_time >= end_time )
        {
            warning("Invalid timings in `animate`"_qs);
            return;
        }

        register_time_range(start_time, end_time);

        std::vector<ValueVariant> values;

        if ( motion )
        {
            if ( !animate.hasAttribute("path"_qs) )
            {
                warning("Missing path for animateMotion"_qs);
                return;
            }

            if ( animate.hasAttribute("rotate"_qs) )
            {
                QString rotate = animate.attribute("rotate"_qs);
                if ( rotate == "auto"_qs )
                {
                    prop.auto_orient = true;
                }
                else
                {
                    bool is_number = false;
                    int degrees = rotate.toInt(&is_number) % 360;
                    if ( !is_number || degrees != 0 )
                    {
                        warning("The only supported values for animateMotion.rotate are auto or 0"_qs);
                        prop.auto_orient = rotate == "auto-reverse"_qs;
                    }
                }
            }

            auto mbez = PathDParser(animate.attribute("path"_qs)).parse();

            /// \todo Automatically add Hold transitions for sub-bezier end points
            for ( const auto& b : mbez.beziers() )
            {
                for ( const auto& p : b )
                {
                    values.push_back(std::vector<qreal>{p.pos.x(), p.pos.y()});
                    prop.motion.push_back(p);
                }
            }

            /// \todo keyPoints
        }
        else
        {
            values = get_values(animate);
        }

        if ( values.empty() )
            return;

        std::vector<model::FrameTime> times;
        times.reserve(values.size());

        if ( animate.hasAttribute("keyTimes"_qs) )
        {
            auto strings = animate.attribute("keyTimes"_qs).split(frame_separator_re,
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        Qt::SkipEmptyParts
#else
        QString::SkipEmptyParts
#endif
            );
            if ( strings.size() != int(values.size()) )
            {
                warning(QString("`keyTimes` (%1) and `values` (%2) mismatch"_qs).arg(strings.size()).arg(int(values.size())));
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
        QString calc = animate.attribute("calcMode"_qs, "linear"_qs);
        if ( calc == "spline"_qs )
        {
            if ( !animate.hasAttribute("keySplines"_qs) )
            {
                warning("Missing `keySplines`"_qs);
                return;
            }

            auto splines = animate.attribute("keySplines"_qs).split(frame_separator_re,
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        Qt::SkipEmptyParts
#else
        QString::SkipEmptyParts
#endif
            );
            if ( splines.size() != int(values.size()) - 1 )
            {
                warning("Wrong number of `keySplines` values"_qs);
                return;
            }

            transitions.reserve(values.size());
            for ( const auto& spline : splines )
            {
                auto params = split_values(spline);
                if ( params.size() != 4 )
                {
                    warning("Invalid value for `keySplines`"_qs);
                    return;
                }
                transitions.push_back({{params[0], params[1]}, {params[2], params[3]}});
            }
            transitions.emplace_back();
        }
        else
        {
            model::KeyframeTransition def;
            if ( calc == "discrete"_qs )
                def.set_hold(true);
            transitions = std::vector<model::KeyframeTransition>(values.size(), def);
        }

        prop.keyframes.reserve(values.size());

        for ( uint i = 0; i < values.size(); i++ )
            prop.keyframes.push_back({times[i], values[i], transitions[i]});
    }

    void store_animate(const QString& target, const QDomElement& animate)
    {
        stored_animate[target].push_back(animate);
    }

    template<class FuncT>
    AnimatedProperties parse_animated_elements(const QDomElement& parent, const FuncT& func)
    {
        AnimatedProperties props;
        props.element = parent;

        for ( const auto& child: ElementRange(parent) )
            func(child, props);

        if ( parent.hasAttribute("id"_qs) )
        {
            auto it = stored_animate.find(parent.attribute("id"_qs));
            if ( it != stored_animate.end() )
            {
                for ( const auto& child: it->second )
                    func(child, props);
            }
        }

        return props;
    }


    AnimatedProperties parse_animated_properties(const QDomElement& parent)
    {
        return parse_animated_elements(parent, [this](const QDomElement& child, AnimatedProperties& props){
            if ( child.tagName() == "animate"_qs && child.hasAttribute("attributeName"_qs) )
                parse_animate(child, props.properties[child.attribute("attributeName"_qs)], false);
            else if ( child.tagName() == "animateMotion"_qs )
                parse_animate(child, props.properties["motion"_qs], true);
        });
    }

    AnimatedProperties parse_animated_transform(const QDomElement& parent)
    {
        return parse_animated_elements(parent, [this](const QDomElement& child, AnimatedProperties& props){
            if ( child.tagName() == "animateTransform"_qs && child.hasAttribute("type"_qs) && child.attribute("attributeName"_qs) == "transform"_qs )
                parse_animate(child, props.properties[child.attribute("type"_qs)], false);
            else if ( child.tagName() == "animateMotion"_qs )
                parse_animate(child, props.properties["motion"_qs], true);
        });
    }

    void warning(const QString& msg)
    {
        if ( on_warning )
            on_warning(msg);
    }

    qreal fps = 60;
    qreal min_kf = 0;
    qreal max_kf = 0;
    bool kf_range_initialized = false;
    std::function<void(const QString&)> on_warning;
    std::unordered_map<QString, std::vector<QDomElement>> stored_animate;
    static const QRegularExpression separator;
    static const QRegularExpression clock_re;
    static const QRegularExpression frame_separator_re;
};

} // namespace glaxnimate::io::svg::detail
