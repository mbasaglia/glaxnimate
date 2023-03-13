/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "utils.hpp"


std::vector<std::unique_ptr<glaxnimate::model::KeyframeBase>> glaxnimate::io::split_keyframes(model::AnimatableBase* prop)
{
    std::vector<std::unique_ptr<model::KeyframeBase>> split_kfs;
    std::unique_ptr<model::KeyframeBase> previous = prop->keyframe(0)->clone();

    for ( int i = 1, e = prop->keyframe_count(); i < e; i++ )
    {
        if ( previous->transition().hold() )
        {
            split_kfs.push_back(std::move(previous));
            previous = prop->keyframe(i)->clone();
            continue;
        }

        std::array<qreal, 2> raw_splits;
        std::tie(raw_splits[0], raw_splits[1]) = previous->transition().bezier().extrema(1);

        std::vector<qreal> splits;
        for ( qreal t : raw_splits )
        {
            QPointF p = previous->transition().bezier().solve(t);
            if ( p.y() < 0 || p.y() > 1 )
                splits.push_back(t);
        }

        if ( splits.size() == 0 )
        {
            split_kfs.push_back(std::move(previous));
            previous = prop->keyframe(i)->clone();
        }
        else
        {
            auto next = prop->keyframe(i);
            auto next_segment = previous->split(next, splits);
            previous = std::move(next_segment.back());
            split_kfs.insert(split_kfs.end(), std::make_move_iterator(next_segment.begin()), std::make_move_iterator(next_segment.end() - 1));
        }
    }

    split_kfs.push_back(std::move(previous));

    return split_kfs;
}

