#include "animatable.hpp"

#include "command/animation_commands.hpp"
#include "model/object.hpp"
#include "math/bezier/segment.hpp"

bool model::AnimatableBase::assign_from(const model::BaseProperty* prop)
{
    if ( prop->traits().flags != traits().flags || prop->traits().type != traits().type )
        return false;

    const AnimatableBase* other = static_cast<const AnimatableBase*>(prop);

    clear_keyframes();

    if ( !other->animated() )
        return set_value(other->value());

    for ( int i = 0, e = other->keyframe_count(); i < e; i++ )
    {
        const KeyframeBase* kf_other = other->keyframe(i);
        KeyframeBase* kf = set_keyframe(kf_other->time(), kf_other->value());
        if ( kf )
        {
            kf->transition().set_hold(kf_other->transition().hold());
            kf->transition().set_before_handle(kf_other->transition().before_handle());
            kf->transition().set_after_handle(kf_other->transition().after_handle());
        }
    }

    return true;
}

bool model::AnimatableBase::set_undoable(const QVariant& val, bool commit)
{
    if ( !valid_value(val) )
        return false;

    object()->push_command(new command::SetMultipleAnimated(
        tr("Update %1").arg(name()),
        {this},
        {value()},
        {val},
        commit
    ));
    return true;
}

model::AnimatableBase::MidTransition model::AnimatableBase::mid_transition(model::FrameTime time) const
{
    int keyframe_index = this->keyframe_index(time);
    const KeyframeBase* kf_before = this->keyframe(keyframe_index);
    if ( !kf_before )
        return {};

    auto before_time = kf_before->time();

    if ( before_time >= time )
        return {
            MidTransition::SingleKeyframe,
            kf_before->value(),
            {{}, {1, 1}},
            {kf_before->transition().before_handle(), kf_before->transition().after_handle()},
        };


    const KeyframeBase* kf_after = this->keyframe(keyframe_index + 1);

    if ( !kf_after )
        return {
            MidTransition::SingleKeyframe,
            kf_before->value(),
            {kf_before->transition().before_handle(), kf_before->transition().after_handle()},
            {{}, {}},
        };

    auto after_time = kf_after->time();

    if ( after_time <= time )
        return {
            MidTransition::SingleKeyframe,
            kf_after->value(),
            {kf_before->transition().before_handle(), kf_before->transition().after_handle()},
            {kf_after->transition().before_handle(), kf_after->transition().after_handle()},
        };

    qreal x = math::unlerp(before_time, after_time, time);
    return do_mid_transition(kf_before, kf_after, x);
}


model::AnimatableBase::MidTransition model::AnimatableBase::do_mid_transition(
    const model::KeyframeBase* kf_before,
    const model::KeyframeBase* kf_after,
    qreal x
) const
{
    auto orig_transition = kf_before->transition().bezier();
    QPointF handle_b1 = orig_transition.points()[1];
    QPointF handle_b2 = orig_transition.points()[2];

    qreal t = kf_before->transition().bezier_parameter(x);

    if ( t <= 0 )
    {
        return {
            MidTransition::SingleKeyframe,
            kf_before->value(),
            {{}, {1, 1}},
            {kf_before->transition().before_handle(), kf_before->transition().after_handle()},
        };
    }
    else if ( t >= 1 )
    {
        return {
            MidTransition::SingleKeyframe,
            kf_before->value(),
            {kf_before->transition().before_handle(), kf_before->transition().after_handle()},
            {kf_after->transition().before_handle(), kf_after->transition().after_handle()},
        };
    }

    model::AnimatableBase::MidTransition mt;
    mt.type = MidTransition::Middle;
    mt.value = do_mid_transition_value(kf_before, kf_after, x);

    qreal y = kf_before->transition().lerp_factor(x);
    math::bezier::BezierSegment left, right;
    std::tie(left, right) = orig_transition.split(t);

    qreal left_factor_x = 1 / x;
    qreal left_factor_y = 1 / y;
    mt.from_previous.first =  {
        left[1].x() * left_factor_x,
        left[1].y() * left_factor_y
    };
    mt.from_previous.second =  {
        left[2].x() * left_factor_x,
        left[2].y() * left_factor_y
    };

    qreal right_factor_x = 1 / (1-x);
    qreal right_factor_y = 1 / (1-y);
    mt.to_next.first =  {
        (right[1].x() - x) * right_factor_x,
        (right[1].y() - y) * right_factor_y
    };
    mt.to_next.second =  {
        (right[2].x() - x) * right_factor_x,
        (right[2].y() - y) * right_factor_y
    };

    return mt;
}
