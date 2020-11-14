#pragma once

#include <variant>

#include <QDomElement>
#include <QRegularExpression>

#include "model/animation/keyframe_transition.hpp"
#include "model/animation/frame_time.hpp"
#include "model/animation/join_animatables.hpp"

#include "detail.hpp"

namespace io::svg::detail {

class AnimateParser
{
public:
    struct PropertyKeyframe
    {
        model::FrameTime time;
        std::vector<qreal> values;
        model::KeyframeTransition transition;
    };

    struct JoinedPropertyKeyframe
    {
        model::FrameTime time;
        std::vector<std::vector<qreal>> values;
        model::KeyframeTransition transition;
    };

    struct AnimatedProperty
    {
        std::vector<PropertyKeyframe> keyframes;
    };

    struct JoinedProperty
    {
        std::variant<const AnimatedProperty*, const QString*, std::vector<qreal>> prop;
        int index = 0;

        bool at_end() const
        {
            if ( prop.index() == 0 )
                return index + 1 == int(std::get<0>(prop)->keyframes.size());
            return true;
        }

        const PropertyKeyframe* keyframe(int off = 0) const
        {
            return &std::get<0>(prop)->keyframes[index+off];
        }

        template<int i>
        decltype(auto) get() const noexcept { return std::get<i>(prop); }
    };

    struct AnimatedProperties
    {
        std::map<QString, AnimatedProperty> properties;
        QDomElement element;

        bool has(const QString& name)
        {
            return properties.count(name);
        }

        std::vector<JoinedPropertyKeyframe> joined(const std::vector<QString>& prop_names) const
        {
            std::vector<JoinedProperty> props;
            props.reserve(prop_names.size());
            int found = 0;
            for ( const auto& name : prop_names )
            {
                auto it = properties.find(name);
                if ( it == properties.end() )
                {
                    props.push_back({&name});
                }
                else
                {
                    props.push_back({&it->second});
                    found++;
                }
            }

            if ( !found )
                return {};

            for ( auto& p : props )
            {
                if ( p.prop.index() == 1 )
                {
                    if ( !element.hasAttribute(*p.get<1>()) )
                        return {};
                    p.prop = split_values(element.attribute(*p.get<1>()));
                }
            }

            std::vector<JoinedPropertyKeyframe> keyframes;

            bool cont = true;
            while ( cont )
            {
                model::FrameTime time = std::numeric_limits<model::FrameTime>::max();
                for ( const auto& p : props )
                {
                    if ( p.prop.index() == 0 && p.keyframe()->time < time  )
                        time = p.keyframe()->time;
                }

                std::vector<std::vector<qreal>> values;
                values.reserve(props.size());

                std::vector<model::KeyframeTransition> transitions;
                transitions.resize(found);
                cont = false;
                for ( auto& p : props )
                {
                    if ( p.prop.index() == 0 )
                    {
                        auto kf = p.keyframe();
                        if ( p.at_end() || (p.index == 0 && kf->time > time) )
                        {
                            values.push_back(kf->values);
                        }
                        else if ( kf->time == time )
                        {
                            p.index++;
                            values.push_back(kf->values);
                            transitions.push_back(kf->transition);
                            cont = true;
                        }
                        else
                        {
                            auto kf1 = p.keyframe(-1);
                            qreal x = math::unlerp(kf1->time, kf->time, time);
                            qreal t = kf1->transition.lerp_factor(x);
                            values.push_back(math::lerp(kf1->values, kf->values, t));
                            transitions.push_back(kf1->transition.split(x).first);
                            cont = true;
                        }
                    }
                    else
                    {
                        values.push_back(p.get<2>());
                    }
                }

                keyframes.push_back({time, std::move(values), model::JoinAnimatables::Keyframe::mix_transitions(transitions)});
            }

            return keyframes;
        }

    };

    static std::vector<qreal> split_values(const QString& v)
    {
        auto split = v.splitRef(separator, QString::SkipEmptyParts);
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

        if ( !match.capturedRef("unit").isEmpty() )
            return match.capturedRef("timecount").toDouble() * units.at(match.captured("unit")) * fps;

        return (
            match.capturedRef("hours").toDouble() * hours +
            match.capturedRef("minutes").toDouble() * minutes +
            match.capturedRef("seconds").toDouble()
        ) * fps;
    }

    std::vector<std::vector<qreal>> get_values(const QDomElement& animate)
    {
        std::vector<std::vector<qreal>> values;

        QString attr = animate.attribute("attributeName");
        if ( attr == "d" )
        {
            warning("Loading animated `d` is not yet implemented");
            return {};
        }

        if ( animate.hasAttribute("values") )
        {
            auto val_str = animate.attribute("values").split(frame_separator_re, QString::SkipEmptyParts);
            values.reserve(val_str.size());
            for ( const auto& val : val_str )
                values.push_back(split_values(val));

            if ( values.size() < 2 )
            {
                warning("Not enough values in `animate`");
                return {};
            }

            for ( uint i = 1; i < values.size(); i++ )
            {
                if ( values[i].size() != values[0].size() )
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
                values.push_back(split_values(animate.attribute("from")));
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

                values.push_back(split_values(animate.attribute(attr)));
            }

            if ( animate.hasAttribute("to") )
            {
                values.push_back(split_values(animate.attribute("to")));
            }
            else if ( animate.hasAttribute("by") )
            {
                auto by = split_values(animate.attribute("to"));
                if ( by.size() != values[0].size() )
                {
                    warning("Mismatching `by` and `from` in `animate`");
                    return {};
                }

                for ( uint i = 0; i < by.size(); i++ )
                    by[i] += values[0][i];

                values.push_back(std::move(by));
            }
            else
            {
                warning("Missing `to` or `by` in `animate`");
                return {};
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

        std::vector<std::vector<qreal>> values = get_values(animate);
        if ( values.empty() )
            return;

        std::vector<model::FrameTime> times;
        times.reserve(values.size());

        if ( animate.hasAttribute("keyTimes") )
        {
            auto strings = animate.attribute("keyTimes").split(frame_separator_re, QString::SkipEmptyParts);
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

            auto splines = animate.attribute("keySplines").split(frame_separator_re, QString::SkipEmptyParts);
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
                if ( child.tagName() == "animateTransform" )
                {
                    if ( child.hasAttribute("type") && child.attribute("attributeName") == "transform" )
                        parse_animate(child, props.properties[child.attribute("type")]);
                }
                else if ( child.tagName() == "animate" )
                {
                    if ( child.hasAttribute("attributeName") )
                        parse_animate(child, props.properties[child.attribute("attributeName")]);
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

} // namespace io::svg::detail
