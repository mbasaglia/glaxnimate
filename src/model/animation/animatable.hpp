#pragma once

#include <QVariant>
#include <QList>

#include "model/animation/keyframe_transition.hpp"


namespace model {



using FrameTime = double;


class KeyframeBase
{
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
public:
    enum KeyframeStatus
    {
        NotAnimated,    ///< Value is not animated
        Animated,       ///< Value is animated but the given time isn't a keyframe
        IsKeyframe      ///< Value is animated and the given time is a keyframe
    };

    virtual ~AnimatableBase() = default;

    /**
     * \brief Number of keyframes must always be >= 1
     */
    virtual int keyframe_count() const = 0;

    /**
     * \param i Keyframe index
     * \pre \p i in [0, keyframe_count()]
     * \return the Corresponding keyframe
     *
     * keyframe(i).time() < keyframe(j).time() <=> i < j
     */
    virtual const KeyframeBase* keyframe(int i) const = 0;
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
    virtual QVariant value(FrameTime time) const = 0;

    /**
     * \brief Set value
     * \pre !animated()
     * \post value(t) == \p value for all t
     * \return false if the value couldn't be set
     */
    bool set_value(const QVariant& value)
    {
        if ( animated() )
            return false;
        return keyframe(0)->set_value(value);
    }

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
        return keyframe_count() > 0;
    }

    /**
     * \brief Index of the keyframe whose time lays in the transition
     *
     * If all keyframes are after \p time, returns 0
     * This means keyframe(keyframe_index(t)) is always valid
     */
    int keyframe_index(FrameTime time) const
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

    Animatable(reference val) : keyframes_{keyframe_type{0, val}} {}

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

    keyframe_type* add_keyframe(FrameTime time, reference value)
    {
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

private:
    QList<keyframe_type> keyframes_;
};

} // namespace model
