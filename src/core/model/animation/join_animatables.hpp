#pragma once
#include <tuple>
#include <set>

#include "model/animation/animatable.hpp"


namespace model {

namespace detail {

inline void collect_times(std::set<FrameTime>&) {}

template<class Head, class... Tail>
void collect_times(std::set<FrameTime>& times, model::AnimatedProperty<Head>* head, model::AnimatedProperty<Tail>*... tail)
{
    for ( const auto& kf : head )
        times.insert(kf.time());

    collect_times(times, tail...);
}


template<class T>
struct JoinedAnimatableItem
{
    JoinedAnimatableItem(const T& value) : value {value} {}

    T value;
    KeyframeTransition transition;
};

void collect_transition(int& count, QPointF& in, QPointF& out)
{
    in /= count;
    out /= count;
}

template<class Head, class... Tail>
void collect_transition(int& count, QPointF& in, QPointF& out,
                        const JoinedAnimatableItem<Head>& head,
                        const JoinedAnimatableItem<Tail>&... tail)
{
    count++;
    in += head.transition.before_handle();
    out += head.transition.after_handle();
    collect_transition(count, in, out, tail...);
}

template<class... Types, std::size_t... Indices>
void collect_transition_tuple(int& count, QPointF& in, QPointF& out,
                              const std::tuple<JoinedAnimatableItem<Types>...>& items,
                              std::index_sequence<Indices...>)
{
    collect_transition(count, in, out, std::get<Indices>(items)...);
}


template<class... Types, std::size_t... Indices>
std::tuple<Types...> get_current_value(
    const std::tuple<model::AnimatedProperty<Types>*...>& properties,
    std::index_sequence<Indices...>
)
{
    return {
        std::get<Indices>(properties)->get()...
    };
}

} // namespace detail

template<class... Types>
class JoinAnimatables
{
private:
    using MidTransition = model::AnimatableBase::MidTransition;
    using Indices = std::make_index_sequence<sizeof...(Types)>;

    static std::array<MidTransition, sizeof...(Types)> get_mid(
        FrameTime time,
        model::AnimatedProperty<Types>*... properties
    )
    {
        return {
            properties->mid_transition(time)...
        };
    }

public:
    struct Keyframe
    {
        FrameTime time;
        std::tuple<detail::JoinedAnimatableItem<Types>...> items;

        template<int index>
        const auto& value() const
        {
            return std::get<index>(items).value;
        }

        KeyframeTransition transition() const
        {
            int count = 0;
            QPointF in;
            QPointF out;
            collect_transition_tuple(count, in, out, items, Indices{});
            KeyframeTransition ret;
            ret.set_before_handle(in);
            ret.set_after_handle(out);
        }
    };

    using iterator = typename std::vector<Keyframe>::const_iterator;

    JoinAnimatables(model::AnimatedProperty<Types>*... properties)
    : properties{properties...}
    {
        std::set<FrameTime> time_set;
        detail::collect_times(time_set, properties...);
        std::vector<FrameTime> time_vector(time_set.begin(), time_set.end());
        time_set.clear();

        std::vector<std::array<MidTransition, sizeof...(Types)>> mids;
        mids.reserve(time_vector.size());
        for ( FrameTime t : time_vector )
            mids.push_back(get_mid(time, properties...));

        keyframes.reserve(time_vector.size());
        for ( std::size_t i = 0; i < time_vector.size(); i++ )
        {
            keyframes.push_back({time, {mids[i].value.template value<Types>()...}});

            for ( std::size_t j = 0; j < sizeof...(Types); j++ )
            {
                keyframes.back()[j].transition.set_before_handle(mids[i].to_next.first);
                keyframes.back()[j].transition.set_after_handle(mids[i].to_next.second);
                if ( mids[i][j].type == MidTransition::Middle && i > 0 && mids[i-1][j].type != MidTransition::Middle )
                {
                    keyframes[i-1][j].transition.set_before_handle(mids[i].from_previous.first);
                    keyframes[i-1][j].transition.set_after_handle(mids[i].from_previous.second);
                }
            }
        }
    }

    template<int index>
    auto property() const
    {
        return std::get<index>(properties);
    }

    bool animated() const
    {
        return !keyframes.empty();
    }

    auto begin() const
    {
        return keyframes.begin();
    }

    auto end() const
    {
        return keyframes.end();
    }

    std::tuple<Types...> current_value() const
    {
        return detail::get_current_value(properties, Indices{});
    }

private:
    std::tuple<model::AnimatedProperty<Types>*...> properties;
    std::vector<Keyframe> keyframes;

};




} // namespace model
