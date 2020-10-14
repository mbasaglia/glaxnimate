#pragma once

#include "animatable.hpp"
#include "math/bezier/bezier.hpp"

namespace model {

template<>
class Keyframe<math::bezier::Bezier> : public KeyframeBase
{
public:
    using value_type = math::bezier::Bezier;
    using reference = const math::bezier::Bezier&;

    Keyframe(FrameTime time, math::bezier::Bezier value)
        : KeyframeBase(time), value_(std::move(value)) {}

    void set(reference v)
    {
        value_ = v;
    }

    reference get() const
    {
        return value_;
    }

    QVariant value() const override
    {
        return QVariant::fromValue(value_);
    }

    bool set_value(const QVariant&) override
    {
        return false;
    }

    value_type lerp(reference other, double t) const
    {
        return value_.lerp(other, this->transition().lerp_factor(t));
    }

private:
    friend class AnimatedProperty<math::bezier::Bezier>;
    math::bezier::Bezier value_;
};

template<>
class AnimatedProperty<math::bezier::Bezier> : public detail::AnimatedProperty<math::bezier::Bezier>
{
public:
    AnimatedProperty(Object* object, const QString& name,
                   PropertyCallback<void, math::bezier::Bezier> emitter = {})
    : detail::AnimatedProperty<math::bezier::Bezier>(object, name, {}, std::move(emitter))
    {}

    int size() const
    {
        return value_.size();
    }

    bool closed() const
    {
        return value_.closed();
    }

    void set_closed(bool closed)
    {
        value_.set_closed(closed);
        for ( auto& keyframe : keyframes_ )
            keyframe->value_.set_closed(closed);
        value_changed();
        emitter(object(), value_);
    }

private:
    math::bezier::Bezier value_;
};


} // namespace model
