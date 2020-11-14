#include "animation_commands.hpp"

#include "model/document.hpp"

command::SetKeyframe::SetKeyframe(
    model::AnimatableBase* prop,
    model::FrameTime time,
    const QVariant& value,
    bool commit
) : Parent(QObject::tr("Update %1 keyframe at %2").arg(prop->name()).arg(time), commit),
    prop(prop),
    time(time),
    before(prop->value(time)),
    after(value),
    had_before(prop->has_keyframe(time))
{}

void command::SetKeyframe::undo()
{
    if ( had_before )
        prop->set_keyframe(time, before);
    else
        prop->remove_keyframe_at_time(time);

    if ( insert_index > 0 )
        prop->keyframe(insert_index-1)->set_transition(trans_before);
}

void command::SetKeyframe::redo()
{
    if ( !calculated )
    {
        auto mid = prop->mid_transition(time);
        model::AnimatableBase::SetKeyframeInfo info;
        auto kf = prop->set_keyframe(time, after, &info);
        if ( kf && info.insertion && info.index > 0 && info.index + 1 < prop->keyframe_count() )
        {
            if ( mid.type != model::AnimatableBase::MidTransition::Middle )
            {
                insert_index = -1;
            }
            else
            {
                insert_index = info.index;

                auto kf_before = prop->keyframe(info.index - 1);
                trans_before = kf_before->transition();

                left = mid.from_previous;
                right = mid.to_next;
            }
        }
    }
    else
    {
        prop->set_keyframe(time, after, nullptr);
    }

    if ( insert_index > 0 )
    {
        prop->keyframe(insert_index-1)->set_transition(left);
        prop->keyframe(insert_index)->set_transition(right);
    }

}

bool command::SetKeyframe::merge_with(const SetKeyframe& other)
{
    if ( other.prop != prop )
        return false;
    after = other.after;
    return true;
}

command::RemoveKeyframeTime::RemoveKeyframeTime(
    model::AnimatableBase* prop,
    model::FrameTime time
) : QUndoCommand(QObject::tr("Remove %1 keyframe at %2").arg(prop->name()).arg(time)),
    prop(prop),
    time(time),
    before(prop->value(time))
{}

void command::RemoveKeyframeTime::undo()
{
    prop->set_keyframe(time, before);
}

void command::RemoveKeyframeTime::redo()
{
    prop->remove_keyframe_at_time(time);
}


command::SetMultipleAnimated::SetMultipleAnimated(model::AnimatableBase* prop, QVariant after, bool commit)
    : SetMultipleAnimated(
        auto_name(prop),
        {prop},
        {},
        {after},
        commit
    )
{}

command::SetMultipleAnimated::SetMultipleAnimated(
    const QString& name,
    const std::vector<model::AnimatableBase*>& props,
    const QVariantList& before,
    const QVariantList& after,
    bool commit
)
    : Parent(name, commit),
    props(props),
    before(before),
    after(after),
    keyframe_after(props[0]->object()->document()->record_to_keyframe()),
    time(props[0]->time())
{
    bool add_before = before.empty();

    for ( auto prop : props )
    {
        if ( add_before )
            this->before.push_back(prop->value());
        keyframe_before.push_back(prop->has_keyframe(time));
    }
}


command::SetMultipleAnimated::SetMultipleAnimated(const QString& name, bool commit)
    : Parent(name, commit)
{
}

void command::SetMultipleAnimated::push_property(model::AnimatableBase* prop, const QVariant& after_val)
{
    keyframe_after = prop->object()->document()->record_to_keyframe();
    time = prop->time();
    props.push_back(prop);
    before.push_back(prop->value());
    after.push_back(after_val);
    keyframe_before.push_back(prop->has_keyframe(time));
}

void command::SetMultipleAnimated::undo()
{
    for ( int i = 0; i < int(props.size()); i++ )
    {
        auto prop = props[i];
        if ( keyframe_after )
        {
            if ( keyframe_before[i] )
                prop->set_keyframe(time, before[i]);
            else
                prop->remove_keyframe_at_time(time);
        }
        else
        {
            if ( keyframe_before[i] )
                prop->set_keyframe(time, before[i]);
            else if ( !prop->animated() || prop->time() == time )
                prop->set_value(before[i]);
        }
    }
}

void command::SetMultipleAnimated::redo()
{
    for ( int i = 0; i < int(props.size()); i++ )
    {
        auto prop = props[i];
        if ( keyframe_after )
            prop->set_keyframe(time, after[i]);
        else if ( !prop->animated() || prop->time() == time )
            prop->set_value(after[i]);
    }
}


bool command::SetMultipleAnimated::merge_with(const SetMultipleAnimated& other)
{
    if ( other.props.size() != props.size() || keyframe_after != other.keyframe_after || time != other.time )
        return false;

    for ( int i = 0; i < int(props.size()); i++ )
        if ( props[i] != other.props[i] )
            return false;

    after = other.after;
    return true;
}

QString command::SetMultipleAnimated::auto_name(model::AnimatableBase* prop)
{
    bool key_before = prop->has_keyframe(prop->time());
    bool key_after = prop->object()->document()->record_to_keyframe();

    if ( key_after && !key_before )
        return QObject::tr("Add keyframe for %1 at %2").arg(prop->name()).arg(prop->time());

    if ( key_before )
        return QObject::tr("Update %1 at %2").arg(prop->name()).arg(prop->time());

    return QObject::tr("Update %1").arg(prop->name());
}



command::SetKeyframeTransition::SetKeyframeTransition(
        model::AnimatableBase* prop,
        int keyframe_index,
        const model::KeyframeTransition& transition
    )
: QUndoCommand(QObject::tr("Update keyframe transition")),
    prop(prop),
    keyframe_index(keyframe_index),
    undo_value(keyframe()->transition()),
    redo_value(transition)
{
}

command::SetKeyframeTransition::SetKeyframeTransition(
    model::AnimatableBase* prop,
    int keyframe_index,
    model::KeyframeTransition::Descriptive desc,
    const QPointF& point,
    bool before_transition
) : SetKeyframeTransition(prop, keyframe_index, prop->keyframe(keyframe_index)->transition())
{
    if ( desc == model::KeyframeTransition::Custom )
    {
        if ( before_transition )
            redo_value.set_before(point);
        else
            redo_value.set_after(point);
    }
    else
    {
        if ( before_transition )
            redo_value.set_before_descriptive(desc);
        else
            redo_value.set_after_descriptive(desc);
    }
}

void command::SetKeyframeTransition::undo()
{
    keyframe()->set_transition(undo_value);
}

void command::SetKeyframeTransition::redo()
{
    keyframe()->set_transition(redo_value);
}

model::KeyframeBase* command::SetKeyframeTransition::keyframe() const
{
    return prop->keyframe(keyframe_index);
}

command::MoveKeyframe::MoveKeyframe(
    model::AnimatableBase* prop,
    int keyframe_index,
    model::FrameTime time_after
) : QUndoCommand(QObject::tr("Move keyframe")),
    prop(prop),
    keyframe_index_before(keyframe_index),
    time_before(prop->keyframe(keyframe_index)->time()),
    time_after(time_after)
{}

void command::MoveKeyframe::undo()
{
    prop->move_keyframe(keyframe_index_after, time_before);
}

void command::MoveKeyframe::redo()
{
    keyframe_index_after = prop->move_keyframe(keyframe_index_before, time_after);
}
