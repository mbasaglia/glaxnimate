#include "animatable.hpp"

#include "command/animation_commands.hpp"
#include "model/object.hpp"
#include "math/bezier/segment.hpp"

bool glaxnimate::model::AnimatableBase::assign_from(const model::BaseProperty* prop)
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
            kf->set_transition(kf_other->transition());
    }

    return true;
}

bool glaxnimate::model::AnimatableBase::set_undoable(const QVariant& val, bool commit)
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

glaxnimate::model::AnimatableBase::MidTransition glaxnimate::model::AnimatableBase::mid_transition(model::FrameTime time) const
{
    int keyframe_index = this->keyframe_index(time);
    const KeyframeBase* kf_before = this->keyframe(keyframe_index);
    if ( !kf_before )
        return {MidTransition::Invalid, value(), {}, {}};

    auto before_time = kf_before->time();

    if ( before_time >= time )
        return {MidTransition::SingleKeyframe, kf_before->value(), {}, kf_before->transition(),};


    const KeyframeBase* kf_after = this->keyframe(keyframe_index + 1);

    if ( !kf_after )
        return {MidTransition::SingleKeyframe, kf_before->value(), kf_before->transition(), {},};

    auto after_time = kf_after->time();

    if ( after_time <= time )
        return {
            MidTransition::SingleKeyframe,
            kf_after->value(),
            kf_before->transition(),
            kf_after->transition(),
        };

    qreal x = math::unlerp(before_time, after_time, time);
    return do_mid_transition(kf_before, kf_after, x, keyframe_index);
}


glaxnimate::model::AnimatableBase::MidTransition glaxnimate::model::AnimatableBase::do_mid_transition(
    const model::KeyframeBase* kf_before,
    const model::KeyframeBase* kf_after,
    qreal x,
    int index
) const
{
    const auto& beftrans = kf_before->transition();
    if ( beftrans.hold() || (beftrans.before() == QPointF(0, 0) && beftrans.after() == QPointF(1,1)) )
        return {MidTransition::Middle, kf_before->value(), beftrans, beftrans};

    qreal t = beftrans.bezier_parameter(x);

    if ( t <= 0 )
    {
        KeyframeTransition from_previous = {{}, {1, 1}};
        if ( index > 0 )
            from_previous = keyframe(index-1)->transition();

        return {MidTransition::SingleKeyframe, kf_before->value(), from_previous, beftrans};
    }
    else if ( t >= 1 )
    {
        return {MidTransition::SingleKeyframe, kf_before->value(), beftrans, kf_after->transition(),};
    }


    model::AnimatableBase::MidTransition mt;
    mt.type = MidTransition::Middle;
    mt.value = do_mid_transition_value(kf_before, kf_after, x);
    std::tie(mt.from_previous, mt.to_next) = beftrans.split(x);
    return mt;
}
