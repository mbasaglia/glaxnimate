#include "animatable.hpp"

#include "command/animation_commands.hpp"

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
    QVariant before = value();
    if ( !set_value(val) )
        return false;

    object()->push_command(new command::SetMultipleAnimated(
        tr("Update %1").arg(name()),
        {this},
        {before},
        {val},
        commit
    ));
    return true;
}
