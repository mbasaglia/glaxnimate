#pragma once

#include <QVariant>
#include <QList>

#include "model/animation/keyframe_transition.hpp"
#include "model/property.hpp"


namespace model {



using FrameTime = double;


class KeyframeBase
{
    Q_GADGET

    Q_PROPERTY(QVariant value READ value)
    Q_PROPERTY(double time READ time)
    Q_PROPERTY(KeyframeTransition& transition READ transition)
public:
    explicit KeyframeBase(FrameTime time) : time_ { time } {}
    virtual ~KeyframeBase() = default;

    virtual QVariant value() const = 0;
    virtual bool set_value(const QVariant& value) = 0;

    FrameTime time() const { return time_; }

    /**
     * \brief Transition into the next value
     */
    const KeyframeTransition& transition() const { return transition_; }

    KeyframeTransition& transition() { return transition_; }

private:
    FrameTime time_;
    KeyframeTransition transition_;
};

class AnimatableBase
{
    Q_GADGET

    Q_PROPERTY(int keyframe_count READ keyframe_count)
    Q_PROPERTY(QVariant value READ value)
    Q_PROPERTY(bool animated READ animated)

public:
    enum KeyframeStatus
    {
        NotAnimated,    ///< Value is not animated
        Animated,       ///< Value is animated but the given time isn't a keyframe
        IsKeyframe,     ///< Value is animated and the given time is a keyframe
        Mismatch        ///< Value is animated and the current value doesn't match the animated value

    };

    virtual ~AnimatableBase() = default;

    /**
     * \brief Number of keyframes
     */
    virtual int keyframe_count() const = 0;

    /**
     * \param i Keyframe index
     * \pre \p i in [0, keyframe_count())
     * \return the Corresponding keyframe or nullptr if not found
     *
     * keyframe(i).time() < keyframe(j).time() <=> i < j
     */
    Q_INVOKABLE virtual const KeyframeBase* keyframe(int i) const = 0;
    virtual KeyframeBase* keyframe(int i) = 0;

    /**
     * \brief Sets a value at a keyframe
     * \post value(time) == \p value && animate() == true
     * \return The keyframe or nullptr if it couldn't be added.
     * If there is already a keyframe at \p time the returned value might be an existing keyframe
     */
    virtual KeyframeBase* add_keyframe(FrameTime time, const QVariant& value) = 0;

    /**
     * \brief Get the value at the given time
     */
    Q_INVOKABLE virtual QVariant value(FrameTime time) const = 0;

    /**
     * \brief Get the current value
     */
    virtual QVariant value() const = 0;

    /**
     * \brief Set the current value
     *
     * If animated(), the value might get overwritten when changing the current time
     */
    virtual bool set_value(const QVariant& value) = 0;

    /**
     * \brief Set the current time
     * \post value() == value(time)
     */
    Q_INVOKABLE virtual void set_time(FrameTime time) = 0;

    /**
     * If animated(), whether the current value has been changed over the animated value
     */
    virtual bool value_mismatch() const = 0;

    /**
     * \brief Set the value for the given keyframe
     */
    bool set_keyframe_value(int keyframe_index, const QVariant& value)
    {
        if ( auto kf = keyframe(keyframe_index) )
            return kf->set_value(value);
        return false;
    }

    /**
     * \brief Whether it has multiple keyframes
     */
    bool animated() const
    {
        return keyframe_count() != 0;
    }

    /**
     * \brief Index of the keyframe whose time lays in the transition
     * \pre animated()
     *
     * If all keyframes are after \p time, returns 0
     * This means keyframe(keyframe_index(t)) is always valid when animated
     */
    Q_INVOKABLE int keyframe_index(FrameTime time) const
    {
        for ( int i = 0; i < keyframe_count(); i++ )
        {
            if ( keyframe(i)->time() >= time )
                return std::max(0, i-1);
        }
        return 0;
    }

    KeyframeStatus keyframe_status(FrameTime time) const
    {
        if ( !animated() )
            return NotAnimated;
        if ( value_mismatch() )
            return Mismatch;
        if ( keyframe(keyframe_index(time))->time() == time )
            return IsKeyframe;
        return Animated;
    }

};


template<class Type>
class Keyframe : public KeyframeBase
{
public:
    using value_type = Type;
    using reference = const Type&;

    Keyframe(FrameTime time, Type value)
        : KeyframeBase(time), value_(std::move(value)) {}


    void set(reference value)
    {
        value_ =  value;
    }

    reference get() const
    {
        return value_;
    }

    QVariant value() const override
    {
        return QVariant::fromValue(value_);
    }

    bool set_value(const QVariant& val) override
    {
        if ( !val.canConvert(qMetaTypeId<Type>()) )
            return false;
        QVariant converted = val;
        if ( !converted.convert(qMetaTypeId<Type>()) )
            return false;
        set(converted.value<Type>());
        return true;
    }

    value_type lerp(reference other, double t)
    {
        return math::lerp(value_, other, transition().lerp_factor(t));
    }

private:
    Type value_;
};

template<class Type>
class  Animatable : public AnimatableBase
{
public:
    using keyframe_type = Keyframe<Type>;
    using value_type = typename Keyframe<Type>::value_type;
    using reference = typename Keyframe<Type>::reference;

    Animatable(reference val) : value_{val} {}

    int keyframe_count() const override
    {
        return keyframes_.size();
    }

    const keyframe_type* keyframe(int i) const override
    {
        if ( i < 0 || i > keyframes_.size() )
            return nullptr;
        return &keyframes_->at(i);
    }

    keyframe_type* keyframe(int i) override
    {
        if ( i < 0 || i > keyframes_.size() )
            return nullptr;
        return &keyframes_->at(i);
    }

    QVariant value() const override
    {
        return QVariant::fromValue(value_);
    }

    QVariant value(FrameTime time) const override
    {
        return QVariant::fromValue(get_at(time));
    }

    keyframe_type* add_keyframe(FrameTime time, const QVariant& val) override
    {
        if ( !val.canConvert(qMetaTypeId<value_type>()) )
            return nullptr;
        QVariant converted = val;
        if ( !converted.convert(qMetaTypeId<value_type>()) )
            return nullptr;
        return add_keyframe(time, val.value<value_type>());
    }

    bool set_value(const QVariant& val) override
    {
        if ( !val.canConvert(qMetaTypeId<value_type>()) )
            return false;
        QVariant converted = val;
        if ( !converted.convert(qMetaTypeId<value_type>()) )
            return false;
        value_ = val.value<value_type>();
        return true;
    }

    bool set(reference val)
    {
        value_ = val;
        mismatched_ = !keyframes_.empty();
        return true;
    }

    void set_time(FrameTime time) override
    {
        if ( !keyframes_.empty() )
            value_ = get_at(time);
        mismatched_ = false;
    }

    keyframe_type* add_keyframe(FrameTime time, reference value)
    {
        if ( !keyframes_.empty() )
        {
            value_ = value;
            keyframes_.append({0, value});
            if ( time != 0 )
            {
                keyframes_.append({time, value});
                if ( time > 0 )
                    return &keyframes_.at(1);
                keyframes_.swap(0, 1);
            }
            return &keyframes_.at(0);
        }

        int index = keyframe_index(time);
        auto kf = keyframe(index);
        if ( kf->time() == time )
        {
            kf->set(value);
            return kf;
        }

        if ( index == 0 && kf->time() > time )
        {
            keyframes_.prepend({time, value});
            return &keyframes_.front();
        }

        keyframes_.insert(index+1, {time, value});
        return index+1;
    }

    value_type get_at(FrameTime time) const
    {
        if ( keyframes_.empty() )
            return value_;

        const keyframe_type* first = keyframe(0);
        int count = keyframe_count();
        if ( count < 2 || first->time() >= time )
            return first->get();

        // We have at least 2 keyframes and time is after the first keyframe
        int index = keyframe_index(time);
        first = keyframe(index);
        if ( index == count - 1 )
            return first->get();

        const keyframe_type* second = keyframe(index+1);
        double scaled_time = (time - first->time()) / (second->time() - first->time());
        double lerp_factor = first->transition().lerp_factor(scaled_time);
        return first->lerp(second, lerp_factor);
    }

    bool value_mismatch() const override
    {
        return mismatched_;
    }


private:
    value_type value_;
    QList<keyframe_type> keyframes_;
    bool mismatched_ = false;
};


class AnimatedPropertyBase : public BaseProperty
{
public:
    using BaseProperty::BaseProperty;

    virtual AnimatableBase& animatable() = 0;
    virtual const AnimatableBase& animatable() const = 0;

    QVariant value() const override { return animatable().value(); }
    bool set_value(const QVariant& val) override { return animatable().set_value(val); }
    bool set_undoable(const QVariant& val) override;
};

template<class Type>
class AnimatedProperty : public AnimatedPropertyBase
{
public:
    AnimatedProperty(Object* object, const QString& name, const Type& default_value)
        : AnimatedPropertyBase(object, name, PropertyTraits::from_scalar<Type>(false, false, true)),
        anim(default_value)
    {}

    Animatable<Type>& animatable() override { return &anim; }
    const Animatable<Type>& animatable() const override { return &anim; }

private:
    Animatable<Type> anim;
};


} // namespace model
