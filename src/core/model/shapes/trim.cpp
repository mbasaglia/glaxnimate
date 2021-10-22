#include "trim.hpp"

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

glaxnimate::math::bezier::MultiBezier glaxnimate::model::Trim::process(glaxnimate::model::FrameTime t, const math::bezier::MultiBezier& mbez) const
{
    if ( mbez.empty() )
        return {};

    auto offset = this->offset.get_at(t);
    auto start_value = this->start.get_at(t);
    auto start = math::fmod(start_value + offset, 1.f);
    auto end_value = this->end.get_at(t);
    auto end = math::fmod(end_value + offset, 1.f);
    if ( qFuzzyIsNull(end) && end_value > start_value )
        end = 1;

    if ( start == 0 && end == 1 )
        return mbez;

    // Same value
    if ( math::abs(start * 1000 - end * 1000) < 1 )
        return {};

    const int length_steps = 5;

    math::bezier::MultiBezier out;
    auto length_data = mbez.length_data(length_steps);
    auto start_data = length_data.at_ratio(start);
    auto end_data = length_data.at_ratio(end);

    if ( start < end )
    {
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
            auto single_start_data = start_data.child_split();
            auto single_end_data = end_data.child_split();
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
            out.beziers().push_back(b);
            return out;
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
            auto single_start_data = start_data.child_split();
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
            auto single_end_data = end_data.child_split();
            chunk_end(mbez.beziers()[end_data.index], b, single_end_data);
            out.beziers().push_back(b);
        }
    }
    else
    {
        /* Chunk split around the bezier origin
         * [ bez[0] ... bez[end] ... bez[start] ... bez[n] ]
         *   AAAAAAAAAA|BBBB|ccc|ddd|eee|FFFFFF|GGGGGGGGGG
         *
         * Nothe that in this case even if start == end, we don't need to do anything special
         */

        /* Special case to keep just 1 bezier for closed paths
         * It simply combines the start/end chunks
         */
        if ( mbez.size() == 1 && mbez.beziers()[0].closed() )
        {
            math::bezier::Bezier b;
            auto single_start_data = start_data.child_split();
            auto single_end_data = end_data.child_split();
            chunk_start(mbez.beziers()[start_data.index], b, single_start_data);
            chunk_end(mbez.beziers()[end_data.index], b, single_end_data, 1);
            out.beziers().push_back(b);
            return out;
        }

        /* we skip the "e" part and get the "F" part
         * [ seg[0] ... seg[single_start] ... seg[m] ]
         *   eeeeeeeeeeeeee|FFFFFFFFFFFFFFFFFFFFFFFF
         */
        {
            math::bezier::Bezier b;
            auto single_start_data = start_data.child_split();
            chunk_start(mbez.beziers()[start_data.index], b, single_start_data);
            out.beziers().push_back(b);
        }

        // Get everything until the end ("G" part)
        for ( int i = start_data.index + 1; i < mbez.size(); i++ )
            out.beziers().push_back(mbez.beziers()[i]);

        // Get everything until end_data ("A" part)
        for ( int i = 0; i < end_data.index; i++ )
            out.beziers().push_back(mbez.beziers()[i]);

        /* we get the "B" part and skip the "c" part
         * [ seg[0] ... seg[single_start] ... seg[m] ]
         *   BBBBBBBBBBBBBBBBBBBBBBB|ccccccccccccccc
         */
        if ( end_data.ratio > 0 )
        {
            math::bezier::Bezier b;
            auto single_end_data = end_data.child_split();
            chunk_end(mbez.beziers()[end_data.index], b, single_end_data);
            out.beziers().push_back(b);
        }
    }

    return out;
}
