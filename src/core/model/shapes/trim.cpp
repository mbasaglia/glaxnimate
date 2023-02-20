/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "trim.hpp"

#include "math/bezier/bezier_length.hpp"
#include "model/shapes/group.hpp"
#include "model/shapes/path.hpp"

#include "model/animation/join_animatables.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Trim)

QIcon glaxnimate::model::Trim::static_tree_icon()
{
    return QIcon::fromTheme("edit-cut");
}

QString glaxnimate::model::Trim::static_type_name_human()
{
    return tr("Trim Path");
}

bool glaxnimate::model::Trim::process_collected() const
{
    return multiple.get() == Simultaneously;
}

static void chunk_start(const glaxnimate::math::bezier::Bezier& in, glaxnimate::math::bezier::Bezier& out, const glaxnimate::math::bezier::LengthData::SplitInfo& split, int max = -1)
{
    using namespace glaxnimate::math::bezier;

    if ( max == -1 )
        max = in.closed_size();

    // empty
    if ( split.ratio == 0 && split.index == 0 && max == in.closed_size() )
    {
        out = in;
        return;
    }

    int index = split.index;

    // gotta split mid segment
    if ( split.ratio < 1 && split.ratio > 0 )
    {

        auto split_points = CubicBezierSolver<QPointF>(in.segment(split.index)).split(split.ratio);
        out.push_back(Point(
            split_points.first[3],
            split_points.first[2],
            split_points.second[1],
            Smooth
        ));
        index += 1;

        //we use the tangents from the split for the next point
        if ( index < max )
        {
            out.push_back(Point(
                split_points.second[3],
                split_points.second[2],
                in[index].tan_out,
                in[index].type
            ));
            index += 1;
        }
    }

    for ( int i = index; i < max; i++ )
        out.push_back(in[i]);
}

static void chunk_end(const glaxnimate::math::bezier::Bezier& in, glaxnimate::math::bezier::Bezier& out, const glaxnimate::math::bezier::LengthData::SplitInfo& split, int min = 0)
{
    using namespace glaxnimate::math::bezier;

    if ( split.ratio == 1 && min == 0 )
    {
        out = in;
        return;
    }

    for ( int i = min; i <= split.index; i++ )
        out.push_back(in[i]);

    // gotta split mid segment
    if ( split.ratio > 0 )
    {
        auto split_points = CubicBezierSolver<QPointF>(in.segment(split.index)).split(split.ratio);

        // adjust tangents for the pevious point
        if ( !out.empty() )
            out[out.size()-1].tan_out = split_points.first[1];

        out.push_back(Point(
            split_points.first[3],
            split_points.first[2],
            split_points.second[1],
            Smooth
        ));
    }
}

glaxnimate::math::bezier::MultiBezier glaxnimate::model::Trim::process(
    glaxnimate::model::FrameTime t,
    const math::bezier::MultiBezier& mbez
) const
{
    if ( mbez.empty() )
        return {};

    auto offset = this->offset.get_at(t);
    auto start = this->start.get_at(t);
    auto end = this->end.get_at(t);

    // Normalize Inputs
    offset = math::fmod(offset, 1.f);
    start = math::bound(0.f, start, 1.f) + offset;
    end = math::bound(0.f, end, 1.f) + offset;
    if ( end < start )
        std::swap(start, end);

    // Handle the degenerate cases
    if ( math::abs(start * 1000 - end * 1000) < 1 )
        return {};

    if ( qFuzzyIsNull(start) && qFuzzyCompare(end, 1.f) )
        return mbez;

    // Get the bezier chunk ratios
    // Note that now 0 <= s < e <= 2
    struct Chunk
    {
        float start;
        float end;
    };
    std::vector<Chunk> chunks;
    if ( end <= 1 )
    {
        // Simplest case, the segment is in [0, 1]
        chunks.push_back({start, end});
    }
    else if ( start > 1 )
    {
        // The whole segment is outside [0, 1]
        chunks.push_back({start - 1, end - 1});
    }
    else
    {
        // The segment goes over the end point, so we need two splits
        chunks.push_back({start, 1});
        chunks.push_back({0, end - 1});
    }


    const int length_steps = 5;
    math::bezier::MultiBezier out;
    math::bezier::LengthData length_data(mbez, length_steps);

    for ( const auto& chunk : chunks )
    {
        auto start_data = length_data.at_ratio(chunk.start);
        auto end_data = length_data.at_ratio(chunk.end);

        /* Chunk of a single curve
         *
         * [ bez[0] ... bez[start==end] ... bez[n] ]
         *              aa|BBCCCCDDD|ee
         *
         * [ seg[0] ... seg[single_start] ... seg[single_end]  ... seg[m] ]
         *              aaaaa|BBBBBBBBBBB|CCC|DDDDDDDD|eeeeee
         */
        if ( start_data.index == end_data.index )
        {
            auto single_start_data = start_data.descend();
            auto single_end_data = end_data.descend();

            math::bezier::Bezier b;

            /**
             * Same bezier segment
             * [ seg[0] ... seg[single_start=single_end]  ... seg[m] ]
             *              aaaaa|BBBBBBBBBBBBBBBB|ccccc
             */
            if ( single_start_data.index == single_end_data.index )
            {
                const auto& in = mbez.beziers()[start_data.index];
                int index = single_start_data.index;
                // split the segment at start
                qreal ratio_start = single_start_data.ratio;
                auto truncated_segment = math::bezier::CubicBezierSolver<QPointF>(in.segment(index)).split(ratio_start).second;
                // find the end ratio for the truncated segment and split it there
                qreal ratio_end = (single_end_data.ratio - ratio_start) / (1-ratio_start);
                auto result = math::bezier::CubicBezierSolver<QPointF>(truncated_segment).split(ratio_end).first;

                // add to the bezier
                b.push_back(math::bezier::Point(
                    result[0],
                    result[0],
                    result[1],
                    math::bezier::Corner
                ));

                b.push_back(math::bezier::Point(
                    result[3],
                    result[2],
                    result[3],
                    math::bezier::Corner
                ));
            }
            else
            {
                int start_max = single_end_data.index;
                if ( single_end_data.index == single_start_data.index + 1 )
                    start_max += 1;

                chunk_start(mbez.beziers()[start_data.index], b, single_start_data, start_max);
                int end_min = qMax(start_max, single_start_data.index + 1);
                chunk_end(mbez.beziers()[start_data.index], b, single_end_data, end_min);

            }
            if ( !b.empty() && !out.beziers().empty() && !out.back().empty() &&
                out.back().back().pos == b[0].pos )
            {
                auto& out_bez = out.back();
                out_bez.back().tan_out = b[0].tan_out;
                out_bez.back().type = math::bezier::Corner;
                out_bez.points().insert(out_bez.end(), b.begin() + 1, b.end());
            }
            else
            {
                out.beziers().push_back(b);
            }
            continue;
        }

        /* Sequential chunk
         *
         * [ bez[0] ... bez[start] ... bez[end] ... bez[n] ]
         *              aa|BBBBBBB|CCC|DDDDD|ee
         */
        out.beziers().reserve(end_data.index - start_data.index);


        /* we skip the "a" part and get the "B" part
         * [ seg[0] ... seg[single_start] ... seg[m] ]
         *   aaaaaaaaaaaaaaa|BBBBBBBBBBBBBBBBBBBBBBB
         */
        {
            math::bezier::Bezier b;
            auto single_start_data = start_data.descend();
            chunk_start(mbez.beziers()[start_data.index], b, single_start_data);
            out.beziers().push_back(b);
        }

        // We get the segment between start and end ("C" part)
        for ( int i = start_data.index + 1; i < end_data.index; i++ )
        {
            out.beziers().push_back(mbez.beziers()[i]);
        }

        /* we get the "D" part and skip the "e" part
         * [ seg[0] ... seg[single_start] ... seg[m] ]
         *   DDDDDDDDDDDDDDDDDDDDDDD|eeeeeeeeeeeeeee
         */
        if ( end_data.ratio > 0 )
        {
            math::bezier::Bezier b;
            auto single_end_data = end_data.descend();
            chunk_end(mbez.beziers()[end_data.index], b, single_end_data);
            out.beziers().push_back(b);
        }
    }

    for ( auto& bez : out.beziers() )
    {
        if ( !bez.empty() && math::fuzzy_compare(bez[0].pos, bez.back().pos) )
        {
            bez[0].tan_in = bez.back().tan_in;
            bez.points().pop_back();
            bez.close();
        }
    }

    return out;
}
