#pragma once

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
            prop->set_keyframe(time, before);
        else
            prop->remove_keyframe_at_time(time);
    }
    
    void redo() override
    {
        prop->set_keyframe(time, after);
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
        prop->set_keyframe(time, before);
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

/**
 * \brief Command that sets multiple animated properties at once, 
 * setting keyframes based on the document record_to_keyframe
 */
class SetMultipleAnimated : public MergeableCommand<Id::SetMultipleAnimated, SetMultipleAnimated>
{
public:    
    SetMultipleAnimated(model::AnimatableBase* prop, QVariant after, bool commit)
        : SetMultipleAnimated(
            auto_name(prop),
            {prop},
            {},
            {after},
            commit
        )
    {}
    
    template<class... Args>
    SetMultipleAnimated(
        const QString& name,
        bool commit,
        const std::vector<model::AnimatableBase*>& props,
        Args... vals
    ) : SetMultipleAnimated(name, props, {}, {QVariant::fromValue(vals)...}, commit)
    {}

    /**
     * \pre props.size() == after.size() && (props.size() == before.size() || before.empty())
     *
     * If before.empty() it will be populated by the properties
     */
    SetMultipleAnimated(
        const QString& name,
        const std::vector<model::AnimatableBase*>& props,
        const QVariantList& before,
        const QVariantList& after,
        bool commit
    )
        : Parent(name, commit), props(props), before(before), after(after), 
        keyframe_after(props[0]->object()->document()->record_to_keyframe()),
        time(props[0]->time())
    {
        bool add_before = before.empty();
        
        for ( auto prop : props )
        {
            if ( add_before )
                this->before.push_back(prop->value());
            keyframe_before.push_back(prop->keyframe_status(time) != model::AnimatableBase::Tween);
        }
    }

    void undo() override
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

    void redo() override
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


    bool merge_with(const SetMultipleAnimated& other)
    {
        if ( other.props.size() != props.size() || keyframe_after != other.keyframe_after || time != other.time )
            return false;

        for ( int i = 0; i < int(props.size()); i++ )
            if ( props[i] != other.props[i] )
                return false;

        after = other.after;
        return true;
    }

private:
    static QString auto_name(model::AnimatableBase* prop) 
    {
        bool key_before = prop->keyframe_status(prop->time()) != model::AnimatableBase::Tween;
        bool key_after = prop->object()->document()->record_to_keyframe();
        
        if ( key_after && !key_before )
            return QObject::tr("Add keyframe for %1 at %2").arg(prop->name()).arg(prop->time());
            
        if ( key_before )
            return QObject::tr("Update %1 at %2").arg(prop->name()).arg(prop->time());
        
        return QObject::tr("Update %1").arg(prop->name());        
    }
    
    std::vector<model::AnimatableBase*> props;
    QVariantList before;
    QVariantList after;
    std::vector<int> keyframe_before;
    bool keyframe_after;
    model::FrameTime time;
};

} // namespace command
