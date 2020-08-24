#pragma once

#include <QVariant>
#include <QList>

#include "model/animation/keyframe_transition.hpp"
#include "model/property.hpp"


namespace model {

class KeyframeBase
{
    Q_GADGET

    Q_PROPERTY(QVariant value READ value)
    Q_PROPERTY(double time READ time)
public:
    explicit KeyframeBase(FrameTime time) : time_ { time } {}
    virtual ~KeyframeBase() = default;

    virtual QVariant value() const = 0;
    virtual bool set_value(const QVariant& value) = 0;
    virtual QVariant extra_variant() const = 0;
    virtual bool set_extra_variant(const QVariant&) = 0;

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

class AnimatableBase : public QObject, public BaseProperty
{
    Q_OBJECT

    Q_PROPERTY(int keyframe_count READ keyframe_count)
    Q_PROPERTY(QVariant value READ value)
    Q_PROPERTY(bool animated READ animated)

public:
    enum KeyframeStatus
    {
        NotAnimated,    ///< Value is not animated
        Tween,          ///< Value is animated but the given time isn't a keyframe
        IsKeyframe,     ///< Value is animated and the given time is a keyframe
        Mismatch        ///< Value is animated and the current value doesn't match the animated value
    };

    using BaseProperty::BaseProperty;

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
    Q_INVOKABLE virtual KeyframeBase* set_keyframe(FrameTime time, const QVariant& value) = 0;
    
    /**
     * \brief Removes the keyframe at index \p i
     */
    Q_INVOKABLE virtual void remove_keyframe(int i) = 0;
    
    /**
     * \brief Removes all keyframes
     * \post !animated()
     */
    Q_INVOKABLE virtual void clear_keyframes() = 0;
    
    /**
     * \brief Removes the keyframe with the given time
     * \returns whether a keyframe was found and removed
     */
    Q_INVOKABLE virtual bool remove_keyframe_at_time(FrameTime time) = 0;

    /**
     * \brief Get the value at the given time
     */
    Q_INVOKABLE virtual QVariant value(FrameTime time) const = 0;
    
    using BaseProperty::value;

    virtual QVariant extra_variant() const = 0;
    virtual bool set_extra_variant(const QVariant&) = 0;

    /**
     * If animated(), whether the current value has been changed over the animated value
     */
    virtual bool value_mismatch() const = 0;
    
    bool assign_from(const BaseProperty* prop) override;
    
    /**
     * \brief Set the current time
     * \post value() == value(time)
     */
    void set_time(FrameTime time)
    {
        current_time = time;
        on_set_time(time);
    }
    
    FrameTime time() const
    {
        return current_time;
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
        auto kfcount = keyframe_count();
        
        for ( int i = 0; i < kfcount; i++ )
        {
            auto kftime = keyframe(i)->time();
            if ( kftime == time )
                return i;
            else if ( kftime > time )
                return std::max(0, i-1);
        }
        return kfcount - 1;
    }

    KeyframeStatus keyframe_status(FrameTime time) const
    {
        if ( !animated() )
            return NotAnimated;
        if ( value_mismatch() )
            return Mismatch;
        if ( keyframe(keyframe_index(time))->time() == time )
            return IsKeyframe;
        return Tween;
    }
    
signals:
    void keyframe_added(int index, KeyframeBase* keyframe);
    void keyframe_removed(int index);
    void keyframe_updated(int index, KeyframeBase* keyframe);
    
protected:
    virtual void on_set_time(FrameTime time) = 0;
    
private:
    FrameTime current_time = 0;
};

namespace detail {
    template<class ExtraDataType>
    class KeyframeExtra : public KeyframeBase
    {
    public:
        using extra_type = ExtraDataType;
        using KeyframeBase::KeyframeBase;

        QVariant extra_variant() const override { return QVariant::fromValue(extra()); }
        bool set_extra_variant(const QVariant& val) override
        {
            if ( auto v = detail::variant_cast<ExtraDataType>(val) )
            {
                set_extra(*v);
                return true;
            }
            return false;
        }

        const ExtraDataType& extra() const { return extra_; }
        void set_extra(const ExtraDataType& v) { extra_ = v; }

    protected:
        ExtraDataType extra_;
    };

    template<class Type>
    class KeyframeWithExtra : public KeyframeBase
    {
    public:
        using KeyframeBase::KeyframeBase;

        QVariant extra_variant() const override { return {}; }
        bool set_extra_variant(const QVariant&) override { return false; }

    protected:
        void extra_data_reset() {}
    };

    template<>
    class KeyframeWithExtra<QColor> : public KeyframeExtra<QString>
    {
    public:
        using KeyframeExtra<QString>::KeyframeExtra;

    protected:
        void extra_data_reset()
        {
            set_extra("");
        }
    };

    template<>
    class KeyframeWithExtra<QPointF> : public KeyframeExtra<QPair<QPointF, QPointF>>
    {
    public:
        using KeyframeExtra<QPair<QPointF, QPointF>>::KeyframeExtra;

    protected:
        void extra_data_reset()  {}
    };
} // namespace detail


template<class Type>
class Keyframe : public detail::KeyframeWithExtra<Type>
{
public:
    using value_type = Type;
    using reference = const Type&;

    Keyframe(FrameTime time, Type value)
        : detail::KeyframeWithExtra<Type>(time), value_(std::move(value)) {}


    void set(reference value)
    {
        value_ =  value;
        this->extra_data_reset();
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
        if ( auto v = detail::variant_cast<Type>(val) )
        {
            set(*v);
            return true;
        }
        return false;
    }

    value_type lerp(reference other, double t) const
    {
        return math::lerp(value_, other, this->transition().lerp_factor(t));
    }

private:
    Type value_;
};

namespace detail {
    template<class ExtraDataType>
    class AnimatableExtra : public AnimatableBase
    {
    public:
        using extra_type = ExtraDataType;
        using AnimatableBase::AnimatableBase;

        QVariant extra_variant() const override { return QVariant::fromValue(extra()); }
        bool set_extra_variant(const QVariant& val) override
        {
            if ( auto v = detail::variant_cast<ExtraDataType>(val) )
            {
                set_extra(*v);
                return true;
            }
            return false;
        }

        const ExtraDataType& extra() const { return extra_; }
        void set_extra(const ExtraDataType& v) { extra_ = v; }

    protected:
        ExtraDataType extra_;
    };


    template<class Type>
    class AnimatableWithExtra : public AnimatableBase
    {
    public:
        using AnimatableBase::AnimatableBase;

        QVariant extra_variant() const override { return {}; }
        bool set_extra_variant(const QVariant&) override { return false; }

    protected:
        void extra_data_reset() {}
        void extra_data_reset(const Keyframe<Type>*) {}
    };

    template<>
    class AnimatableWithExtra<QColor> : public AnimatableExtra<QString>
    {
    public:
        using AnimatableExtra<QString>::AnimatableExtra;

    protected:
        void extra_data_reset()
        {
            set_extra("");
        }
        void extra_data_reset(const Keyframe<QColor>* kf)
        {
            if ( kf )
                set_extra(kf->extra());
        }
    };



} // namespace detail

template<class Type>
class  AnimatedProperty : public detail::AnimatableWithExtra<Type>
{
public:
    using keyframe_type = Keyframe<Type>;
    using value_type = typename Keyframe<Type>::value_type;
    using reference = typename Keyframe<Type>::reference;

    AnimatedProperty(Object* object, const QString& name, reference default_value)
    : detail::AnimatableWithExtra<Type>(object, name, PropertyTraits::from_scalar<Type>(PropertyTraits::Animated|PropertyTraits::Visual)),
      value_{default_value} {}

    int keyframe_count() const override
    {
        return keyframes_.size();
    }

    const keyframe_type* keyframe(int i) const override
    {
        if ( i < 0 || i > int(keyframes_.size()) )
            return nullptr;
        return keyframes_[i].get();
    }

    keyframe_type* keyframe(int i) override
    {
        if ( i < 0 || i >= int(keyframes_.size()) )
            return nullptr;
        return keyframes_[i].get();
    }

    QVariant value() const override
    {
        return QVariant::fromValue(value_);
    }

    QVariant value(FrameTime time) const override
    {
        return QVariant::fromValue(get_at(time));
    }

    keyframe_type* set_keyframe(FrameTime time, const QVariant& val) override
    {
        if ( auto v = detail::variant_cast<Type>(val) )
            return set_keyframe(time, *v);
        return nullptr;
    }
    
    void remove_keyframe(int i) override
    {
        if ( i > 0 && i <= int(keyframes_.size()) )
        {
            keyframes_.erase(keyframes_.begin() + i);
            emit this->keyframe_removed(i);
        }
    }
    
    void clear_keyframes() override
    {
        int n = keyframes_.size();
        keyframes_.clear();
        for ( int i = 0; i < n; i++ )
            emit this->keyframe_removed(i);
    }
    
    bool remove_keyframe_at_time(FrameTime time) override
    {
        for ( auto it = keyframes_.begin(); it != keyframes_.end(); ++it )
        {
            if ( (*it)->time() == time )
            {
                keyframes_.erase(it);
                emit this->keyframe_removed(it - keyframes_.begin());
                return true;
            }
        }
        return false;
    }

    bool set_value(const QVariant& val) override
    {
        if ( auto v = detail::variant_cast<Type>(val) )
            return set(*v);
        return false;
    }

    bool set(reference val)
    {
        value_ = val;
        mismatched_ = !keyframes_.empty();
        this->extra_data_reset();
        this->value_changed();
        return true;
    }

    keyframe_type* set_keyframe(FrameTime time, reference value)
    {
        // First keyframe
        if ( keyframes_.empty() )
        {
            value_ = value;
            this->value_changed();
            keyframes_.push_back(std::make_unique<keyframe_type>(time, value));
            emit this->keyframe_added(0, keyframes_.back().get());
            return keyframes_.back().get();
        }
        
        // Current time, update value_
        if ( time == this->time() )
        {
            value_ = value;
            this->value_changed();
        }

        // Find the right keyframe
        int index = this->keyframe_index(time);
        auto kf = keyframe(index);
                
        // Time matches, update
        if ( kf->time() == time )
        {
            kf->set(value);
            emit this->keyframe_updated(index, kf);
            return kf;
        }

        // First keyframe not at 0, might have to add the new keyframe at 0
        if ( index == 0 && kf->time() > time )
        {
            keyframes_.insert(keyframes_.begin(), std::make_unique<keyframe_type>(time, value));
            emit this->keyframe_added(0, keyframes_.front().get());
            return keyframes_.front().get();
        }

        // Insert somewhere in the middle
        auto it = keyframes_.insert(
            keyframes_.begin() + index + 1,
            std::make_unique<keyframe_type>(time, value)
        );
        emit this->keyframe_added(index + 1, it->get());
        return it->get();
    }

    value_type get() const
    {
        return value_;
    }

    value_type get_at(FrameTime time) const
    {
        return get_at_impl(time).second;
    }

    bool value_mismatch() const override
    {
        return mismatched_;
    }
    
protected:
    void on_set_time(FrameTime time) override
    {
        if ( !keyframes_.empty() )
        {
            const keyframe_type* kf;
            std::tie(kf, value_) = get_at_impl(time);
            this->extra_data_reset(kf);
            this->value_changed();
        }
        mismatched_ = false;
    }
    
private:
    std::pair<const keyframe_type*, value_type> get_at_impl(FrameTime time) const
    {
        if ( keyframes_.empty() )
            return {nullptr, value_};

        const keyframe_type* first = keyframe(0);
        int count = keyframe_count();
        if ( count < 2 || first->time() >= time )
            return {first, first->get()};

        // We have at least 2 keyframes and time is after the first keyframe
        int index = this->keyframe_index(time);
        first = keyframe(index);
        if ( index == count - 1 || first->time() == time )
            return {first, first->get()};

        const keyframe_type* second = keyframe(index+1);
        double scaled_time = (time - first->time()) / (second->time() - first->time());
        double lerp_factor = first->transition().lerp_factor(scaled_time);
        return {nullptr, first->lerp(second->get(), lerp_factor)};
    }

    value_type value_;
    std::vector<std::unique_ptr<keyframe_type>> keyframes_;
    bool mismatched_ = false;
};

} // namespace model
