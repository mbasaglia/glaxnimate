#include "base.hpp"

#include "command/base.hpp"
#include "model/animation/animatable.hpp"

namespace command {
    
class SetKeyframe : public MergeableCommand<Id::SetKeyframe, SetKeyframe>
{
public:
    SetKeyframe(
        model::AnimatableBase* prop, 
        model::FrameTime time, 
        const QVariant& value,
        bool commit
    ) : Parent(QObject::tr("Update %1 keyframe at %2").arg(prop->name()).arg(time), commit),
        prop(prop),
        time(time),
        before(prop->value(time)),
        after(value),
        had_before(prop->keyframe_status(time) != model::AnimatableBase::Tween)
    {}
    
    void undo() override
    {
        if ( had_before )
            prop->add_keyframe(time, before);
        else
            prop->remove_keyframe_at_time(time);
    }
    
    void redo() override
    {
        prop->add_keyframe(time, after);
    }
    
    bool merge_with(const SetKeyframe& other)
    {
        if ( other.prop != prop )
            return false;
        after = other.after;
        return true;
    }
    
private:
    model::AnimatableBase* prop;
    model::FrameTime time;
    QVariant before;
    QVariant after;
    bool had_before;
};

class RemoveKeyframe : public QUndoCommand
{
public:
    RemoveKeyframe(
        model::AnimatableBase* prop, 
        model::FrameTime time
    ) : QUndoCommand(QObject::tr("Remove %1 keyframe at %2").arg(prop->name()).arg(time)),
        prop(prop),
        time(time),
        before(prop->value(time))
    {}
    
    void undo() override
    {
        prop->add_keyframe(time, before);
    }
    
    void redo() override
    {
        prop->remove_keyframe_at_time(time);
    }
    
private:
    model::AnimatableBase* prop;
    model::FrameTime time;
    QVariant before;
};

} // namespace command
