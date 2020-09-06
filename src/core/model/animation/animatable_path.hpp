#pragma once

#include "animatable.hpp"
#include "math/bezier.hpp"

namespace model {

template<>
class Keyframe<math::Bezier> : public KeyframeBase
{
public:
    using value_type = math::Bezier;
    using reference = const math::Bezier&;

    Keyframe(FrameTime time, math::Bezier value)
        : KeyframeBase(time), value_(std::move(value)) {}

    void set(reference) {}

    reference get() const
    {
        return value_;
    }

    QVariant extra_variant() const override { return {}; }
    bool set_extra_variant(const QVariant&) override { return false; }

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
    friend class AnimatablePath;
    math::Bezier value_;
};

class AnimatablePath : public AnimatedProperty<math::Bezier>
{
public:
    AnimatablePath(Object* object, const QString& name,
                   PropertyCallback<void, math::Bezier> emitter = {})
    : AnimatedProperty(object, name, {}, std::move(emitter))
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

    void add_point(int index, qreal factor)
    {
        value_.split_segment(index, factor);
        for ( auto& keyframe : keyframes_ )
            keyframe->value_.split_segment(index, factor);
        value_changed();
        emitter(object(), value_);
    }

    void remove_point(int index)
    {
        value_.remove_point(index);
        for ( auto& keyframe : keyframes_ )
            keyframe->value_.remove_point(index);
        value_changed();
        emitter(object(), value_);
    }

    void move_point(int index, const QPointF& pos)
    {
        value_.points()[index].pos = pos;
        value_changed();
    }

    void move_tan_in(int index, const QPointF& tan_in)
    {
        value_.points()[index].tan_in = tan_in;
        value_changed();
    }

    void move_tan_out(int index, const QPointF& tan_out)
    {
        value_.points()[index].tan_out = tan_out;
        value_changed();
    }

private:
    math::Bezier value_;
};

} // namespace model
