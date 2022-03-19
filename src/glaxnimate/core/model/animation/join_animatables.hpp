#pragma once
#include <tuple>
#include <set>

#include "glaxnimate/core/model/animation/animatable.hpp"


namespace glaxnimate::model {

/**
 * \brief Utility to join keyframes from multiple animatables
 */
class JoinAnimatables
{
private:
    using MidTransition = model::AnimatableBase::MidTransition;

public:
    struct Keyframe
    {
        FrameTime time;
        std::vector<QVariant> values;
        std::vector<KeyframeTransition> transitions;

        Keyframe(FrameTime time, std::size_t prop_count)
            : time(time)
        {
            values.reserve(prop_count);
            transitions.reserve(prop_count);
        }

        static KeyframeTransition mix_transitions(const std::vector<KeyframeTransition>& transitions)
        {
            int count = 0;
            QPointF in;
            QPointF out;
            for ( const auto& transition : transitions )
            {
                if ( !transition.hold() )
                {
                    in += transition.before();
                    out += transition.after();
                    count++;
                }
            }

            if ( count == 0 )
                return {{0, 0}, {1, 1}, true};

            return {in / count, out / count};
        }

        KeyframeTransition transition() const
        {
            return mix_transitions(transitions);
        }
    };

    enum Flags
    {
        Normal      = 0x00,
        NoKeyframes = 0x01,
        NoValues    = 0x02,
    };

    using iterator = typename std::vector<Keyframe>::const_iterator;

    JoinAnimatables(std::vector<const model::AnimatableBase*> properties, int flags = Normal)
    : properties_(std::move(properties))
    {
        if ( !(flags & NoKeyframes) )
            load_keyframes(flags);
    }

    /**
     * \brief Whether the result has more than one keyframe
     */
    bool animated() const
    {
        return keyframes_.size() > 1;
    }

    /**
     * \brief Keyframe range begin
     */
    auto begin() const
    {
        return keyframes_.begin();
    }

    /**
     * \brief Keyframe range end
     */
    auto end() const
    {
        return keyframes_.end();
    }

    /**
     * \brief Current value as a vector of variants
     */
    std::vector<QVariant> current_value() const
    {
        std::vector<QVariant> values;
        values.reserve(properties_.size());
        for ( auto prop : properties_ )
            values.push_back(prop->value());
        return values;
    }

    /**
     * \brief Value at time as a vector of variants
     */
    std::vector<QVariant> value_at(FrameTime time) const
    {
        std::vector<QVariant> values;
        values.reserve(properties_.size());
        for ( auto prop : properties_ )
            values.push_back(prop->value(time));
        return values;
    }

    const std::vector<const model::AnimatableBase*>& properties() const
    {
        return properties_;
    }

    const std::vector<Keyframe>& keyframes() const
    {
        return keyframes_;
    }

    /**
     * \brief Current value combined by a callback
     * \pre each property can be converted to the corresponding \p AnimatedProperty<Args>.
     */
    template<class... Args, class Func>
    auto combine_current_value(const Func& func)
    {
        return invoke_combine_get<Args...>(func, std::index_sequence_for<Args...>());
    }

    /**
     * \brief Value at a given time combined by a callback
     * \pre each property can be converted to the corresponding \p AnimatedProperty<Args>.
     */
    template<class... Args, class Func>
    auto combine_value_at(FrameTime time, const Func& func)
    {
        return invoke_combine_get_at<Args...>(time, func, std::index_sequence_for<Args...>());
    }

    /**
     * \brief Fills \p target with the combined values
     * \pre Each property can be converted to the corresponding \p AnimatedProperty<Args>.
     * \pre \p target values can be initialized by what \p func returns
     */
    template<class... Args, class Target, class Func>
    void apply_to(Target* target, const Func& func, const model::AnimatedProperty<Args>*...)
    {
        target->clear_keyframes();
        target->set(combine_current_value<Args...>(func));
        for ( const auto& keyframe : keyframes_ )
        {
            auto real_kf = target->set_keyframe(keyframe.time, combine_value_at<Args...>(keyframe.time, func));
            real_kf->set_transition(keyframe.transition());
        }
    }

private:
    std::vector<const model::AnimatableBase*> properties_;
    std::vector<Keyframe> keyframes_;

    void load_keyframes(int flags)
    {
        std::set<FrameTime> time_set;
        for ( auto prop : properties_ )
            for ( int i = 0, e = prop->keyframe_count(); i < e; i++ )
                time_set.insert(prop->keyframe(i)->time());
        std::vector<FrameTime> time_vector(time_set.begin(), time_set.end());
        time_set.clear();

        std::vector<std::vector<MidTransition>> mids;
        mids.reserve(time_vector.size());
        for ( FrameTime t : time_vector )
        {
            mids.push_back({});
            mids.back().reserve(properties_.size());
            for ( auto prop : properties_ )
                mids.back().push_back(prop->mid_transition(t));
        }

        keyframes_.reserve(time_vector.size());
        for ( std::size_t i = 0; i < time_vector.size(); i++ )
        {
            keyframes_.emplace_back(time_vector[i], properties_.size());

            for ( std::size_t j = 0; j < properties_.size(); j++ )
            {
                if ( !(flags & NoValues) )
                    keyframes_.back().values.push_back(mids[i][j].value);
                keyframes_.back().transitions.push_back(mids[i][j].to_next);
                if ( mids[i][j].type == MidTransition::Middle && i > 0 && mids[i-1][j].type != MidTransition::Middle )
                {
                    keyframes_[i-1].transitions[j] = mids[i][j].from_previous;
                }
            }
        }
    }

    template<class... Args, class Func, std::size_t... Indices>
    auto invoke_combine_get(const Func& func, std::integer_sequence<std::size_t, Indices...>)
    {
        return func(static_cast<const model::AnimatedProperty<Args>*>(properties_[Indices])->get()...);
    }

    template<class... Args, class Func, std::size_t... Indices>
    auto invoke_combine_get_at(FrameTime t, const Func& func, std::integer_sequence<std::size_t, Indices...>)
    {
        return func(static_cast<const model::AnimatedProperty<Args>*>(properties_[Indices])->get_at(t)...);
    }
};

/**
 * \brief JoinAnimatables implementing AnimatableBase
 */
class JoinedAnimatable : public AnimatableBase, public JoinAnimatables
{
public:
    using ConversionFunction = std::function<QVariant (const std::vector<QVariant> args)>;

    class Keyframe : public KeyframeBase
    {
    public:
        Keyframe(JoinedAnimatable* parent, const JoinAnimatables::Keyframe* subkf)
            : KeyframeBase(subkf->time),
              parent(parent),
              subkf(subkf)
        {
            set_transition(subkf->transition());
        }

        QVariant value() const override
        {
            return parent->converter(subkf->values);
        }

        // read only
        bool set_value(const QVariant&) override { return false; }

    private:
        JoinedAnimatable* parent;
        const JoinAnimatables::Keyframe* subkf;
    };

    JoinedAnimatable(std::vector<const model::AnimatableBase*> properties, ConversionFunction converter, int flags = Normal)
        : AnimatableBase(nullptr, "", {}),
          JoinAnimatables(std::move(properties), flags),
          converter(std::move(converter))

    {
        wrapped_keyframes.reserve(keyframes().size());
        for ( auto& kf : keyframes() )
            wrapped_keyframes.push_back(std::make_unique<Keyframe>(this, &kf));
    }

    int keyframe_count() const override
    {
        return wrapped_keyframes.size();
    }

    using JoinAnimatables::animated;

    const KeyframeBase* keyframe(int i) const override
    {
        return wrapped_keyframes[i].get();
    }

    KeyframeBase* keyframe(int i) override
    {
        return wrapped_keyframes[i].get();
    }

    QVariant value(FrameTime time) const override
    {
        return converter(value_at(time));
    }

    QVariant value() const override
    {
        return converter(current_value());
    }


    // read only
    bool set_value(const QVariant&) override { return false; }
    bool valid_value(const QVariant&) const override { return false; }
    bool set_undoable(const QVariant&, bool) override { return false; }
    int move_keyframe(int, FrameTime) override { return -1; }
    bool value_mismatch() const override { return false; }
    KeyframeBase* set_keyframe(FrameTime , const QVariant& , SetKeyframeInfo* ) override { return nullptr; }
    void remove_keyframe(int) override {};
    void clear_keyframes() override {};
    bool remove_keyframe_at_time(FrameTime) override { return false; }

protected:
    void on_set_time(FrameTime) override {}

    // Shouldn't be needed
    QVariant do_mid_transition_value(const KeyframeBase*, const KeyframeBase*, qreal) const override
    {
        return {};
    }


private:
    ConversionFunction converter;
    std::vector<std::unique_ptr<Keyframe>> wrapped_keyframes;
};

} // namespace glaxnimate::model
