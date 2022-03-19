#pragma once

#include <set>

#include "animatable.hpp"
#include "glaxnimate/math/bezier/bezier.hpp"

namespace glaxnimate::model {

namespace detail {
class AnimatedPropertyBezier;
} // namespace glaxnimate::name



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
    Q_INVOKABLE void remove_point(int index);
    void remove_points(const std::set<int>& indices);

    /**
     * \brief Extends all keyframe values to match \p target
     *
     * Each keyframe adds nodes from \p target to have at least \p target.size() nodes
     */
    void extend(const math::bezier::Bezier& target, bool at_end);
};

} // namespace detail

template<>
class AnimatedProperty<math::bezier::Bezier> : public detail::AnimatedPropertyBezier
{
public:
    using detail::AnimatedPropertyBezier::AnimatedPropertyBezier;
};


} // namespace glaxnimate::model
