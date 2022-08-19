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

namespace glaxnimate::io::svg::detail {

class AnimateParser
{
public:
    struct ValueVariant
    {
    public:
        enum Type
        {
            Vector, Bezier
        };

        ValueVariant(std::vector<qreal> v = {})
            : value_(std::move(v))
        {}

        ValueVariant(math::bezier::MultiBezier v )
            : value_(std::move(v))
        {}

        Type type() const { return Type(value_.index()); }

        const std::vector<qreal>& vector() const
        {
            return std::get<int(Type::Vector)>(value_);
        }

        const math::bezier::MultiBezier& bezier() const
        {
            return std::get<int(Type::Bezier)>(value_);
        }

        ValueVariant lerp(const ValueVariant& other, qreal t) const
        {
            if ( type() != other.type() )
                return *this;

            switch ( type() )
            {
                case Type::Vector:
                    return math::lerp(vector(), other.vector(), t);
                case Type::Bezier:
                    if ( bezier().size() == 1 && other.bezier().size() == 1 )
                    {
                        math::bezier::MultiBezier mb;
                        mb.beziers().push_back(bezier()[0].lerp(other.bezier()[0], t));
                        return mb;
                    }
                    return *this;
            }

            return {};
        }

        bool compatible(const ValueVariant& other) const
        {
            if ( type() != other.type() )
                return false;

            if ( type() == Vector )
                return vector().size() == other.vector().size();

            return true;
        }

    private:
        std::variant<std::vector<qreal>, math::bezier::MultiBezier> value_;
    };

    struct PropertyKeyframe
    {
        model::FrameTime time;
        ValueVariant values;
        model::KeyframeTransition transition;
    };

    struct JoinedPropertyKeyframe
    {
        model::FrameTime time;
        std::vector<ValueVariant> values;
        model::KeyframeTransition transition;
    };

    struct AnimatedProperty
    {
        std::vector<PropertyKeyframe> keyframes;
    };

    struct JoinedProperty
    {
        std::variant<const AnimatedProperty*, const QString*, ValueVariant> prop;
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

        std::vector<PropertyKeyframe> single(const QString& prop_name) const
        {
            auto it = properties.find(prop_name);
            if ( it == properties.end() || it->second.keyframes.size() < 2 )
                return {};
            return it->second.keyframes;
        }

        std::vector<JoinedPropertyKeyframe> joined(const std::vector<QString>& prop_names) const
        {
            std::vector<JoinedProperty> props;
            props.reserve(prop_names.size());
            int found = 0;
            for ( const auto& name : prop_names )
            {
                auto it = properties.find(name);
                if ( it == properties.end() || it->second.keyframes.size() < 2 )
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

                std::vector<ValueVariant> values;
                values.reserve(props.size());

                std::vector<model::KeyframeTransition> transitions;
                transitions.resize(found);
                cont = false;
                for ( auto& p : props )
                {
                    if ( p.prop.index() == 0 )
                    {
                        auto kf = p.keyframe();
                        if ( (p.at_end() && kf->time <= time) || (p.index == 0 && kf->time > time) )
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
                            values.push_back(kf1->values.lerp(kf->values, t));
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
        }

        return {};
    }

    std::vector<ValueVariant> get_values(const QDomElement& animate)
    {
        QString attr = animate.attribute("attributeName");
        ValueVariant::Type type = ValueVariant::Vector;
        if ( attr == "d" )
            type = ValueVariant::Bezier;

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
