/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <variant>
#include "math/bezier/bezier.hpp"
#include "model/animation/keyframe_transition.hpp"
#include "model/animation/frame_time.hpp"
#include "model/animation/join_animatables.hpp"


namespace glaxnimate::io::detail {

struct ValueVariant
{
public:
    enum Type
    {
        Vector, Bezier, String, Color
    };

    ValueVariant(qreal v)
        : value_(std::vector<qreal>{v})
    {}

    ValueVariant(std::vector<qreal> v = {})
        : value_(std::move(v))
    {}

    ValueVariant(math::bezier::MultiBezier v )
        : value_(std::move(v))
    {}

    ValueVariant(QString v)
        : value_(std::move(v))
    {}

    ValueVariant(QColor v)
        : value_(std::move(v))
    {}

    ValueVariant(const QVariant& v)
    {
        if ( v.userType() == QMetaType::QColor )
            value_ = v.value<QColor>();
        else if ( v.userType() == QMetaType::QString )
            value_ = v.value<QString>();
        else if ( v.canConvert<qreal>() )
            value_ = std::vector<qreal>(1, v.toReal());
    }

    Type type() const { return Type(value_.index()); }

    qreal scalar() const
    {
        return vector()[0];
    }

    const std::vector<qreal>& vector() const
    {
        return std::get<int(Type::Vector)>(value_);
    }

    const math::bezier::MultiBezier& bezier() const
    {
        return std::get<int(Type::Bezier)>(value_);
    }

    const QString& string() const
    {
        return std::get<int(Type::String)>(value_);
    }

    const QColor& color() const
    {
        return std::get<int(Type::Color)>(value_);
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
            case Type::String:
                return t < 1 ? string() : other.string();
            case Type::Color:
                return math::lerp(color(), other.color(), t);

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
    std::variant<std::vector<qreal>, math::bezier::MultiBezier, QString, QColor> value_;
};

struct PropertyKeyframe
{
    model::FrameTime time;
    ValueVariant values;
    model::KeyframeTransition transition;

    bool operator< (const PropertyKeyframe& o) const
    {
        return time < o.time;
    }
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
    math::bezier::Bezier motion;

    void sort()
    {
        std::sort(keyframes.begin(), keyframes.end());
    }
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

    virtual ~AnimatedProperties() {}
    virtual bool prepare_joined(std::vector<JoinedProperty>&) const { return true; }

    bool has(const QString& name) const
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

        if ( !prepare_joined(props) )
            return {};

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

} // namespace glaxnimate::io::detail
