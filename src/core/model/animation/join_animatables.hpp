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
        std::vector<QPair<QPointF, QPointF>> transitions;

        Keyframe(FrameTime time, std::size_t prop_count)
            : time(time)
        {
            values.reserve(prop_count);
            transitions.reserve(prop_count);
        }

        QPair<QPointF, QPointF> transition() const
        {
            QPointF in;
            QPointF out;
            for ( const auto& transition : transitions )
            {
                 in += transition.first;
                 out += transition.second;
            }

            in /= transitions.size();
            out /= transitions.size();

            return {in, out};
        }
    };

    using iterator = typename std::vector<Keyframe>::const_iterator;

    JoinAnimatables(std::vector<model::AnimatableBase*> properties)
    : properties_(std::move(properties))
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
            mids.back().resize(properties_.size());
            for ( auto prop : properties_ )
                mids.back().push_back(prop->mid_transition(t));
        }

        keyframes_.reserve(time_vector.size());
        for ( std::size_t i = 0; i < time_vector.size(); i++ )
        {
            keyframes_.emplace_back(time_vector[i], properties_.size());

            for ( std::size_t j = 0; j < properties_.size(); j++ )
            {
                keyframes_.back().values.push_back(mids[i][j].value);
                keyframes_.back().transitions.push_back(mids[i][j].to_next);
                if ( mids[i][j].type == MidTransition::Middle && i > 0 && mids[i-1][j].type != MidTransition::Middle )
                {
                    keyframes_[i-1].transitions[j] = mids[i][j].from_previous;
                }
            }
        }
    }

    bool animated() const
    {
        return !keyframes_.empty();
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

};




} // namespace model
