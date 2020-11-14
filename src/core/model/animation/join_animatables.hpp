#pragma once
#include <tuple>
#include <set>

#include "model/animation/animatable.hpp"


namespace model {

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

        KeyframeTransition transition() const
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
    };

    enum Flags
    {
        Normal      = 0x00,
        NoKeyframes = 0x01,
        NoValues    = 0x02,
    };

    using iterator = typename std::vector<Keyframe>::const_iterator;

    JoinAnimatables(std::vector<model::AnimatableBase*> properties, int flags = Normal)
    : properties_(std::move(properties))
    {
        if ( !(flags & NoKeyframes) )
            load_keyframes(flags);
    }

    bool animated() const
    {
        return keyframes_.size() > 1;
    }

    auto begin() const
    {
        return keyframes_.begin();
    }

    auto end() const
    {
        return keyframes_.end();
    }

    std::vector<QVariant> current_value() const
    {
        std::vector<QVariant> values;
        values.reserve(properties_.size());
        for ( auto prop : properties_ )
            values.push_back(prop->value());
        return values;
    }

    const std::vector<model::AnimatableBase*>& properties() const
    {
        return properties_;
    }

    const std::vector<Keyframe>& keyframes() const
    {
        return keyframes_;
    }

private:
    std::vector<model::AnimatableBase*> properties_;
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

};

} // namespace model
