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

void glaxnimate::model::detail::AnimatedPropertyPosition::split_segment(int index, qreal factor)
{
    auto before = bezier();
    auto after = before;

    after.split_segment(index, factor);
    object()->push_command(new command::SetPositionBezier(
        this, before, after, true, tr("Split Segment")
    ));
}

bool glaxnimate::model::detail::AnimatedPropertyPosition::set_bezier(math::bezier::Bezier bezier)
{
    if ( bezier.closed() && !bezier.empty() && !math::fuzzy_compare(bezier[0].pos, bezier.back().pos) )
        bezier.push_back(bezier[0]);

    // TODO if sizes don't match, re-arrange keyframes based on
    // how far keyframes are in the bezier
    // eg: a point at 50% of the length will result in a keyframe
    // at time (keyframes[0].time + keyframes[-1].time) / 2
    if ( bezier.size() != int(keyframes_.size()) )
        return false;

    for ( int i = 0; i < bezier.size(); i++ )
    {
        keyframes_[i]->set_point(bezier[i]);
        on_keyframe_updated(keyframes_[i]->time(), i-1, i+1);
    }

    value_ = get_at_impl(time()).second;
    emitter(this->object(), value_);
    emit bezier_set(bezier);

    return true;
}

glaxnimate::math::bezier::Bezier glaxnimate::model::detail::AnimatedPropertyPosition::bezier() const
{
    math::bezier::Bezier bez;
    for ( const auto& kf : keyframes_ )
        bez.push_back(kf->point());

    return bez;
}

glaxnimate::model::detail::AnimatedPropertyPosition::keyframe_type*
    glaxnimate::model::detail::AnimatedPropertyPosition::set_keyframe(
        FrameTime time, const QVariant& val, SetKeyframeInfo* info
)
{
    if ( auto v = detail::variant_cast<QPointF>(val) )
        return detail::AnimatedProperty<QPointF>::set_keyframe(time, *v, info);

    // We accept a bezier here so it can be used with SetMultipleAnimated
    if ( auto v = detail::variant_cast<math::bezier::Bezier>(val) )
    {
        set_bezier(*v);
        return nullptr;
    }

    return nullptr;
}

glaxnimate::model::detail::AnimatedPropertyPosition::keyframe_type*
    glaxnimate::model::detail::AnimatedPropertyPosition::set_keyframe(
        FrameTime time, reference value, SetKeyframeInfo* info
)
{
    return detail::AnimatedProperty<QPointF>::set_keyframe(time, value, info);
}

bool glaxnimate::model::detail::AnimatedPropertyPosition::set_value(const QVariant& val)
{
    if ( auto v = detail::variant_cast<QPointF>(val) )
        return detail::AnimatedProperty<QPointF>::set(*v);

    if ( auto v = detail::variant_cast<math::bezier::Bezier>(val) )
        return set_bezier(*v);

    return false;
}

bool glaxnimate::model::detail::AnimatedPropertyPosition::valid_value(const QVariant& val) const
{
    if ( detail::variant_cast<QPointF>(val) || detail::variant_cast<math::bezier::Bezier>(val) )
        return true;
    return false;
}
