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
    {
        prop->keyframe(insert_index-1)->transition().set_before_handle(handle_b1);
        prop->keyframe(insert_index-1)->transition().set_after_handle(handle_b2);
    }
}

void command::SetKeyframe::redo()
{
    model::AnimatableBase::SetKeyframeInfo info;
    auto kf = prop->set_keyframe(time, after, &info);
    if ( !calculated )
    {
        if ( kf && info.insertion && info.index > 0 && info.index + 1 < prop->keyframe_count() )
        {
            insert_index = info.index;
            auto kf_before = prop->keyframe(info.index - 1);
            auto kf_after = prop->keyframe(info.index + 1);
            auto orig_transition = kf_before->transition().bezier();
            handle_b1 = orig_transition.points()[1];
            handle_b2 = orig_transition.points()[2];

            qreal x = math::unlerp(kf_before->time(), kf_after->time(), kf->time());
            qreal t = kf_before->transition().bezier_parameter(x);
            if ( t == 0 )
            {
                handle_l1 = handle_r1 = kf_before->transition().before_handle();
                handle_l2 = handle_r2 = kf_before->transition().after_handle();
            }
            else if ( t < 1 )
            {
                qreal y = kf_before->transition().lerp_factor(x);
                math::bezier::BezierSegment left, right;
                std::tie(left, right) = orig_transition.split(t);

                qreal left_factor_x = 1 / x;
                qreal left_factor_y = 1 / y;
                handle_l1 =  {
                    left[1].x() * left_factor_x,
                    left[1].y() * left_factor_y
                };
                handle_l2 =  {
                    left[2].x() * left_factor_x,
                    left[2].y() * left_factor_y
                };

                qreal right_factor_x = 1 / (1-x);
                qreal right_factor_y = 1 / (1-y);
                handle_r1 =  {
                    (right[1].x() - x) * right_factor_x,
                    (right[1].y() - y) * right_factor_y
                };
                handle_r2 =  {
                    (right[2].x() - x) * right_factor_x,
                    (right[2].y() - y) * right_factor_y
                };
            }
            else
            {
                insert_index = -1;
            }
        }
    }

    if ( insert_index > 0 )
    {
        prop->keyframe(insert_index-1)->transition().set_before_handle(handle_l1);
        prop->keyframe(insert_index-1)->transition().set_after_handle(handle_l2);
        prop->keyframe(insert_index)->transition().set_before_handle(handle_r1);
        prop->keyframe(insert_index)->transition().set_after_handle(handle_r2);
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
    model::KeyframeTransition::Descriptive desc,
    const QPointF& point,
    bool before_transition
) : QUndoCommand(QObject::tr("Update keyframe transition")),
    prop(prop),
    keyframe_index(keyframe_index),
    undo_value(
        before_transition ? keyframe()->transition().before_handle() : keyframe()->transition().after_handle()
    ),
    undo_desc(
        before_transition ? keyframe()->transition().before() : keyframe()->transition().after()
    ),
    redo_value(point),
    redo_desc(desc),
    before_transition(before_transition)
{}

void command::SetKeyframeTransition::undo()
{
    set_handle(undo_value, undo_desc);
}

void command::SetKeyframeTransition::redo()
{
    set_handle(redo_value, redo_desc);
}

model::KeyframeBase* command::SetKeyframeTransition::keyframe() const
{
    return prop->keyframe(keyframe_index);
}

void command::SetKeyframeTransition::set_handle(const QPointF& v, model::KeyframeTransition::Descriptive desc) const
{
    if ( desc == model::KeyframeTransition::Custom )
    {
        if ( before_transition )
            keyframe()->transition().set_before_handle(v);
        else
            keyframe()->transition().set_after_handle(v);
    }
    else
    {
        if ( before_transition )
            keyframe()->transition().set_before(desc);
        else
            keyframe()->transition().set_after(desc);
    }
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
