#pragma once

#include "animatable.hpp"
#include "math/bezier/bezier.hpp"

namespace model {

namespace detail {
class AnimatedPropertyBezier;
} // namespace name



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
    friend detail::AnimatedPropertyBezier;
    math::bezier::Bezier value_;
};

namespace detail {

// Intermediare non-templated class so Q_OBJECT works
class AnimatedPropertyBezier : public detail::AnimatedProperty<math::bezier::Bezier>
{
    Q_OBJECT
public:
    AnimatedPropertyBezier(Object* object, const QString& name,
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

    void set_closed(bool closed);

    Q_INVOKABLE void split_segment(int index, qreal factor);
};

} // namespace detail

template<>
class AnimatedProperty<math::bezier::Bezier> : public detail::AnimatedPropertyBezier
{
public:
    using detail::AnimatedPropertyBezier::AnimatedPropertyBezier;
};


} // namespace model
