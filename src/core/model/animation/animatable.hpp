#pragma once

#include <limits>
#include <iterator>

#include <QVariant>
#include <QList>

#include "model/animation/keyframe_transition.hpp"
#include "model/property/property.hpp"
#include "math/math.hpp"
#include "math/bezier/point.hpp"
#include "math/bezier/solver.hpp"
#include "math/bezier/bezier_length.hpp"

/*
 * Arguments: type, name, default, emitter, flags
 * For float: type, name, default, emitter, min, max, flags
 */
#define GLAXNIMATE_ANIMATABLE(type, name, ...)                                  \
public:                                                                         \
    glaxnimate::model::AnimatedProperty<type> name{this, #name, __VA_ARGS__};   \
    glaxnimate::model::AnimatableBase* get_##name() { return &name; }           \
private:                                                                        \
    Q_PROPERTY(glaxnimate::model::AnimatableBase* name READ get_##name)         \
    Q_CLASSINFO(#name, "property animated " #type)                              \
    // macro end


namespace glaxnimate::model {

class KeyframeBase : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariant value READ value)
    Q_PROPERTY(double time READ time)
public:
    explicit KeyframeBase(FrameTime time) : time_ { time } {}
    virtual ~KeyframeBase() = default;

    virtual QVariant value() const = 0;
    virtual bool set_value(const QVariant& value) = 0;

    FrameTime time() const { return time_; }
    void set_time(FrameTime t) { time_ = t; }

    /**
     * \brief Transition into the next value
     */
    const KeyframeTransition& transition() const { return transition_; }

    void set_transition(const KeyframeTransition& trans)
    {
        transition_ = trans;
        emit transition_changed(transition_.before_descriptive(), transition_.after_descriptive());
    }

    void stretch_time(qreal multiplier)
    {
        time_ *= multiplier;
    }

signals:
    void transition_changed(KeyframeTransition::Descriptive before, KeyframeTransition::Descriptive after);

private:
    FrameTime time_;
    KeyframeTransition transition_;
};

class AnimatableBase : public QObject, public BaseProperty
{
    Q_OBJECT

    Q_PROPERTY(int keyframe_count READ keyframe_count)
    Q_PROPERTY(QVariant value READ value WRITE set_undoable)
    Q_PROPERTY(bool animated READ animated)

public:
    enum KeyframeStatus
    {
        NotAnimated,    ///< Value is not animated
        Tween,          ///< Value is animated but the given time isn't a keyframe
        IsKeyframe,     ///< Value is animated and the given time is a keyframe
        Mismatch        ///< Value is animated and the current value doesn't match the animated value
    };

    struct SetKeyframeInfo
    {
        bool insertion;
        int index;
    };

    struct MidTransition
    {
        enum Type
        {
            Invalid,
            SingleKeyframe,
            Middle,
        };
        Type type = Invalid;

        QVariant value;
        model::KeyframeTransition from_previous;
        model::KeyframeTransition to_next;
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
    virtual const KeyframeBase* keyframe(int i) const = 0;
    virtual KeyframeBase* keyframe(int i) = 0;

    /**
     * \brief Sets a value at a keyframe
     * \param time          Time to set the value at
     * \param value         Value to set
     * \param info          If not nullptr, it will be written to with information about what has been node
     * \param force_insert  If \b true, it will always add a new keyframe
     * \post value(time) == \p value && animate() == true
     * \return The keyframe or nullptr if it couldn't be added.
     * If there is already a keyframe at \p time the returned value might be an existing keyframe
     */
    virtual KeyframeBase* set_keyframe(FrameTime time, const QVariant& value, SetKeyframeInfo* info = nullptr, bool force_insert = false) = 0;

    /**
     * \brief Removes the keyframe at index \p i
     */
    virtual void remove_keyframe(int i) = 0;

    /**
     * \brief Removes all keyframes
     * \post !animated()
     */
    virtual void clear_keyframes() = 0;

    /**
     * \brief Removes the keyframe with the given time
     * \returns whether a keyframe was found and removed
     */
    virtual bool remove_keyframe_at_time(FrameTime time) = 0;

    /**
     * \brief Get the value at the given time
     */
    virtual QVariant value(FrameTime time) const = 0;

    bool set_undoable(const QVariant& val, bool commit=true) override;

    using BaseProperty::value;

    /**
     * \brief Moves a keyframe
     * \param keyframe_index Index of the keyframe to move
     * \param time New time for the keyframe
     * \return The new index for that keyframe
     */
    virtual int move_keyframe(int keyframe_index, FrameTime time) = 0;

    /**
     * If animated(), whether the current value has been changed over the animated value
     */
    Q_INVOKABLE virtual bool value_mismatch() const = 0;

    bool assign_from(const BaseProperty* prop) override;

    /**
     * \brief Set the current time
     * \post value() == value(time)
     */
    void set_time(FrameTime time) override
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
    Q_INVOKABLE int keyframe_index(double time) const
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

    int keyframe_index(KeyframeBase* kf) const
    {
        auto kfcount = keyframe_count();

        for ( int i = 0; i < kfcount; i++ )
        {
            if ( keyframe(i) == kf )
                return i;
        }
        return -1;
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

    bool has_keyframe(FrameTime time) const
    {
        if ( !animated() )
            return false;
        return keyframe(keyframe_index(time))->time() == time;
    }

    MidTransition mid_transition(FrameTime time) const;

signals:
    void keyframe_added(int index, KeyframeBase* keyframe);
    void keyframe_removed(int index);
    void keyframe_updated(int index, KeyframeBase* keyframe);

protected:
    virtual void on_set_time(FrameTime time) = 0;

    MidTransition do_mid_transition(const KeyframeBase* kf_before, const KeyframeBase* kf_after, qreal ratio, int index) const;
    virtual QVariant do_mid_transition_value(const KeyframeBase* kf_before, const KeyframeBase* kf_after, qreal ratio) const = 0;

    FrameTime current_time = 0;
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
        value_ = value;
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

    value_type lerp(const Keyframe& other, double t) const
    {
        return math::lerp(value_, other.get(), this->transition().lerp_factor(t));
    }

private:
    Type value_;
};


template<>
class Keyframe<QPointF> : public KeyframeBase
{
public:
    using value_type = QPointF;
    using reference = const QPointF&;

    Keyframe(FrameTime time, const QPointF& value)
        : KeyframeBase(time), point_(value) {}

    void set(reference value)
    {
        point_.translate_to(value);
    }

    reference get() const
    {
        return point_.pos;
    }

    QVariant value() const override
    {
        return QVariant::fromValue(get());
    }

    bool set_value(const QVariant& val) override
    {
        if ( auto v = detail::variant_cast<QPointF>(val) )
        {
            set(*v);
            return true;
        }
        return false;
    }

    value_type lerp(const Keyframe& other, double t) const
    {
        auto factor = transition().lerp_factor(t);
        if ( linear && other.linear )
            return math::lerp(get(), other.get(), factor);

        math::bezier::CubicBezierSolver<QPointF> solver(
            point_.pos, point_.tan_out, other.point_.tan_in, other.point_.pos
        );
        math::bezier::LengthData len(solver, 20);
        return solver.solve(len.at_ratio(factor).ratio);
    }

    void set_point(const math::bezier::Point& point)
    {
        point_ = point;
        linear = point_.tan_in == point.pos && point.tan_out == point.pos;
    }

    const math::bezier::Point& point() const
    {
        return point_;
    }

private:
    math::bezier::Point point_;
    bool linear = true;
};

template<class Type>
class AnimatedProperty;

namespace detail {

template<class Type>
class AnimatedProperty : public AnimatableBase
{
public:
    using keyframe_type = Keyframe<Type>;
    using value_type = typename Keyframe<Type>::value_type;
    using reference = typename Keyframe<Type>::reference;

    class iterator
    {
    public:
        using value_type = keyframe_type;
        using pointer = const value_type*;
        using reference = const value_type&;
        using difference_type = int;
        using iterator_category = std::random_access_iterator_tag;

        iterator(const AnimatedProperty* prop, int index) noexcept
        : prop(prop), index(index) {}

        reference operator*() const { return *prop->keyframes_[index]; }
        pointer operator->() const { return prop->keyframes_[index].get(); }

        bool operator==(const iterator& other) const noexcept
        {
            return prop == other.prop && index == other.index;
        }
        bool operator!=(const iterator& other) const noexcept
        {
            return prop != other.prop || index != other.index;
        }
        bool operator<=(const iterator& other) const noexcept
        {
            return prop < other.prop || (prop == other.prop && index <= other.index);
        }
        bool operator<(const iterator& other) const noexcept
        {
            return prop < other.prop || (prop == other.prop && index < other.index);
        }
        bool operator>=(const iterator& other) const noexcept
        {
            return prop > other.prop || (prop == other.prop && index >= other.index);
        }
        bool operator>(const iterator& other) const noexcept
        {
            return prop > other.prop || (prop == other.prop && index > other.index);
        }

        iterator& operator++() noexcept
        {
            index++;
            return *this;
        }
        iterator operator++(int) noexcept
        {
            auto copy = this;
            index++;
            return copy;
        }
        iterator& operator--() noexcept
        {
            index--;
            return *this;
        }
        iterator operator--(int) noexcept
        {
            auto copy = this;
            index--;
            return copy;
        }
        iterator& operator+=(difference_type d) noexcept
        {
            index += d;
            return *this;
        }
        iterator operator+(difference_type d) const noexcept
        {
            auto copy = this;
            return copy += d;
        }
        iterator& operator-=(difference_type d) noexcept
        {
            index -= d;
            return *this;
        }
        iterator operator-(difference_type d) const noexcept
        {
            auto copy = this;
            return copy -= d;
        }
        difference_type operator-(const iterator& other) const noexcept
        {
            return index - other.index;
        }

    private:
        const AnimatedProperty* prop;
        int index;
    };

    AnimatedProperty(
        Object* object,
        const QString& name,
        reference default_value,
        PropertyCallback<void, Type> emitter = {},
        int flags = 0
    )
    : AnimatableBase(
        object, name, PropertyTraits::from_scalar<Type>(
            PropertyTraits::Animated|PropertyTraits::Visual|flags
        )),
      value_{default_value},
      emitter(std::move(emitter))
    {}

    int keyframe_count() const override
    {
        return keyframes_.size();
    }

    const keyframe_type* keyframe(int i) const override
    {
        if ( i < 0 || i >= int(keyframes_.size()) )
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

    keyframe_type* set_keyframe(FrameTime time, const QVariant& val, SetKeyframeInfo* info = nullptr, bool force_insert = false) override
    {
        if ( auto v = detail::variant_cast<Type>(val) )
            return static_cast<model::AnimatedProperty<Type>*>(this)->set_keyframe(time, *v, info, force_insert);
        return nullptr;
    }

    void remove_keyframe(int i) override
    {
        if ( i >= 0 && i <= int(keyframes_.size()) )
        {
            keyframes_.erase(keyframes_.begin() + i);
            emit this->keyframe_removed(i);
            value_changed();
        }
    }

    void clear_keyframes() override
    {
        int n = keyframes_.size();
        keyframes_.clear();
        for ( int i = n - 1; i >= 0; i-- )
            emit this->keyframe_removed(i);
    }

    bool remove_keyframe_at_time(FrameTime time) override
    {
        for ( auto it = keyframes_.begin(); it != keyframes_.end(); ++it )
        {
            if ( (*it)->time() == time )
            {
                int index = it - keyframes_.begin();
                keyframes_.erase(it);
                emit this->keyframe_removed(index);
                on_keyframe_updated(time, index-1, index);
                return true;
            }
        }
        return false;
    }

    bool set_value(const QVariant& val) override
    {
        if ( auto v = detail::variant_cast<Type>(val) )
            return static_cast<model::AnimatedProperty<Type>*>(this)->set(*v);
        return false;
    }

    bool valid_value(const QVariant& val) const override
    {
        if ( detail::variant_cast<Type>(val) )
            return true;
        return false;
    }

    bool set(reference val)
    {
        value_ = val;
        mismatched_ = !keyframes_.empty();
        this->value_changed();
        emitter(this->object(), value_);
        return true;
    }

    keyframe_type* set_keyframe(FrameTime time, reference value, SetKeyframeInfo* info = nullptr, bool force_insert = false)
    {
        // First keyframe
        if ( keyframes_.empty() )
        {
            value_ = value;
            this->value_changed();
            emitter(this->object(), value_);
            keyframes_.push_back(std::make_unique<keyframe_type>(time, value));
            emit this->keyframe_added(0, keyframes_.back().get());
            if ( info )
                *info = {true, 0};
            return keyframes_.back().get();
        }

        // Current time, update value_
        if ( time == this->time() )
        {
            value_ = value;
            this->value_changed();
            emitter(this->object(), value_);
        }

        // Find the right keyframe
        int index = this->keyframe_index(time);
        auto kf = keyframe(index);

        // Time matches, update
        if ( kf->time() == time && !force_insert )
        {
            kf->set(value);
            emit this->keyframe_updated(index, kf);
            on_keyframe_updated(time, index-1, index+1);
            if ( info )
                *info = {false, index};
            return kf;
        }

        // First keyframe not at 0, might have to add the new keyframe at 0
        if ( index == 0 && kf->time() > time )
        {
            keyframes_.insert(keyframes_.begin(), std::make_unique<keyframe_type>(time, value));
            emit this->keyframe_added(0, keyframes_.front().get());
            on_keyframe_updated(time, -1, 1);
            if ( info )
                *info = {true, 0};
            return keyframes_.front().get();
        }

        // Insert somewhere in the middle
        auto it = keyframes_.insert(
            keyframes_.begin() + index + 1,
            std::make_unique<keyframe_type>(time, value)
        );
        emit this->keyframe_added(index + 1, it->get());
        on_keyframe_updated(time, index, index+2);
        if ( info )
            *info = {true, index+1};
        return it->get();
    }

    value_type get() const
    {
        return value_;
    }

    value_type get_at(FrameTime time) const
    {
        if ( time == this->time() )
            return value_;
        return get_at_impl(time).second;
    }

    bool value_mismatch() const override
    {
        return mismatched_;
    }

    int move_keyframe(int keyframe_index, FrameTime time) override
    {
        if ( keyframe_index < 0 || keyframe_index >= int(keyframes_.size()) )
            return keyframe_index;

        int new_index = 0;
        for ( ; new_index < int(keyframes_.size()); new_index++ )
        {
            if ( keyframes_[new_index]->time() > time )
                break;
        }

        if ( new_index > keyframe_index )
            new_index--;

        keyframes_[keyframe_index]->set_time(time);

        if ( keyframe_index != new_index )
        {

            QPointF incoming(-1, -1);
            if ( keyframe_index > 0 )
            {
                auto trans_before_src = keyframes_[keyframe_index - 1]->transition();
                incoming = trans_before_src.after();
                trans_before_src.set_after(keyframes_[keyframe_index]->transition().after());
                keyframes_[keyframe_index - 1]->set_transition(trans_before_src);
            }


            auto move = std::move(keyframes_[keyframe_index]);
            keyframes_.erase(keyframes_.begin() + keyframe_index);
            keyframes_.insert(keyframes_.begin() + new_index, std::move(move));

            int ia = keyframe_index;
            int ib = new_index;
            if ( ia > ib )
                std::swap(ia, ib);

            if ( new_index > 0 )
            {
                auto trans_before_dst = keyframes_[new_index - 1]->transition();
                QPointF outgoing = trans_before_dst.after();

                if ( incoming.x() != -1 )
                {
                    trans_before_dst.set_after(incoming);
                    keyframes_[new_index - 1]->set_transition(trans_before_dst);
                }

                auto trans_moved = keyframes_[new_index]->transition();
                trans_moved.set_after(outgoing);
                keyframes_[new_index]->set_transition(trans_moved);
            }

            for ( ; ia <= ib; ia++ )
                emit this->keyframe_updated(ia, keyframes_[ia].get());
        }
        else
        {
            emit this->keyframe_updated(keyframe_index, keyframes_[keyframe_index].get());
        }

        return new_index;
    }

    iterator begin() const { return iterator{this, 0}; }
    iterator end() const { return iterator{this, int(keyframes_.size())}; }

    void stretch_time(qreal multiplier) override
    {
        for ( std::size_t i = 0; i < keyframes_.size(); i++ )
        {
            keyframes_[i]->stretch_time(multiplier);
            emit keyframe_updated(i, keyframes_[i].get());
        }

        current_time *= multiplier;
    }

protected:
    void on_set_time(FrameTime time) override
    {
        if ( !keyframes_.empty() )
        {
            const keyframe_type* kf;
            std::tie(kf, value_) = get_at_impl(time);
            this->value_changed();
            emitter(this->object(), value_);
        }
        mismatched_ = false;
    }

    void on_keyframe_updated(FrameTime kf_time, int prev_index, int next_index)
    {
        auto cur_time = time();
        // if no keyframes or the current keyframe is being modified => update value_
        if ( !keyframes_.empty() && cur_time != kf_time )
        {
            if ( kf_time > cur_time )
            {
                // if the modified keyframe is far ahead => don't update value_
                if ( prev_index >= 0 && keyframes_[prev_index]->time() > cur_time )
                    return;
            }
            else
            {
                // if the modified keyframe is far behind => don't update value_
                if ( next_index < int(keyframes_.size()) && keyframes_[next_index]->time() < cur_time )
                    return;
            }
        }

        on_set_time(cur_time);
    }

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

        // Only one keyframe needed to get the value
        if ( index == count - 1 || first->time() == time )
            return {first, first->get()};

        // Interpolate between two keyframes
        const keyframe_type* second = keyframe(index+1);
        double scaled_time = (time - first->time()) / (second->time() - first->time());
        double lerp_factor = first->transition().lerp_factor(scaled_time);
        return {nullptr, first->lerp(*second, lerp_factor)};
    }

    QVariant do_mid_transition_value(const KeyframeBase* kf_before, const KeyframeBase* kf_after, qreal ratio) const override
    {
        return QVariant::fromValue(
            static_cast<const keyframe_type*>(kf_before)->lerp(
                *static_cast<const keyframe_type*>(kf_after),
                ratio
            )
        );
    }

    value_type value_;
    std::vector<std::unique_ptr<keyframe_type>> keyframes_;
    bool mismatched_ = false;
    PropertyCallback<void, Type> emitter;
};

// Intermediare non-templated class so Q_OBJECT works
class AnimatedPropertyPosition: public detail::AnimatedProperty<QPointF>
{
    Q_OBJECT
public:
    AnimatedPropertyPosition(
        Object* object,
        const QString& name,
        reference default_value,
        PropertyCallback<void, QPointF> emitter = {},
        int flags = 0
    ) : detail::AnimatedProperty<QPointF>(object, name, default_value, std::move(emitter), flags)
    {
    }


    void set_closed(bool closed);

    Q_INVOKABLE void split_segment(int index, qreal factor);

    Q_INVOKABLE bool set_bezier(math::bezier::Bezier bezier);

    Q_INVOKABLE math::bezier::Bezier bezier() const;

    keyframe_type* set_keyframe(FrameTime time, const QVariant& val, SetKeyframeInfo* info = nullptr, bool force_insert = false) override;

    keyframe_type* set_keyframe(FrameTime time, reference value, SetKeyframeInfo* info = nullptr, bool force_insert = false);

    bool set_value(const QVariant& val) override;

    bool valid_value(const QVariant& val) const override;

signals:
    /// Invoked on set_bezier()
    void  bezier_set(const math::bezier::Bezier& bezier);
};


} // namespace detail


template<class Type>
class AnimatedProperty : public detail::AnimatedProperty<Type>
{
public:
    using detail::AnimatedProperty<Type>::AnimatedProperty;
};


template<>
class AnimatedProperty<float> : public detail::AnimatedProperty<float>
{
public:
    AnimatedProperty(
        Object* object,
        const QString& name,
        reference default_value,
        PropertyCallback<void, float> emitter = {},
        float min = std::numeric_limits<float>::lowest(),
        float max = std::numeric_limits<float>::max(),
        bool cycle = false,
        int flags = 0
    ) : detail::AnimatedProperty<float>(object, name, default_value, std::move(emitter), flags),
        min_(min),
        max_(max),
        cycle_(cycle)
    {
    }

    float max() const { return max_; }
    float min() const { return min_; }

    bool set(reference val)
    {
        return detail::AnimatedProperty<float>::set(bound(val));
    }

    using AnimatableBase::set_keyframe;

    keyframe_type* set_keyframe(FrameTime time, reference value, SetKeyframeInfo* info = nullptr, bool force_insert = false)
    {
        return detail::AnimatedProperty<float>::set_keyframe(time, bound(value), info, force_insert);
    }

private:
    float bound(float value) const
    {
        return cycle_ ?
            math::fmod(value, max_) :
            math::bound(min_, value, max_)
        ;
    }

    float min_;
    float max_;
    bool cycle_;
};


template<>
class AnimatedProperty<QPointF> : public detail::AnimatedPropertyPosition
{
public:
    using detail::AnimatedPropertyPosition::AnimatedPropertyPosition;
};

} // namespace glaxnimate::model
