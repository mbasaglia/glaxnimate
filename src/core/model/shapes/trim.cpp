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
    if ( max == -1 )
        max = in.closed_size();

    if ( split.ratio == 0 && split.index == 0 && max == in.closed_size() )
    {
        out = in;
        return;
    }

    if ( split.ratio < 1 )
        out.push_back(in.split_segment_point(split.index, split.ratio));

    for ( int i = split.index + 1; i < max; i++ )
        out.push_back(in[i]);
}

static void chunk_end(const glaxnimate::math::bezier::Bezier& in, glaxnimate::math::bezier::Bezier& out, const glaxnimate::math::bezier::LengthData::SplitInfo& split, int min = 0)
{

    if ( split.ratio == 1 && min == 0 )
    {
        out = in;
        return;
    }

    for ( int i = min; i <= split.index; i++ )
        out.push_back(in[i]);

    if ( split.ratio > 0 )
        out.push_back(in.split_segment_point(split.index, split.ratio));
}

glaxnimate::math::bezier::MultiBezier glaxnimate::model::Trim::process(glaxnimate::model::FrameTime t, const math::bezier::MultiBezier& mbez) const
{
    if ( mbez.empty() )
        return {};

    auto offset = this->offset.get_at(t);
    auto start = math::fmod(this->start.get_at(t) + offset, 1.f);
    auto end = math::fmod(this->end.get_at(t) + offset, 1.f);
    if ( end == 0 )
        end = 1;

    if ( start == 0 && end == 1 )
        return mbez;

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
            chunk_start(mbez.beziers()[start_data.index], b, single_start_data, single_end_data.index);
            int end_min = qMax(single_end_data.index, single_start_data.index + 1);
            chunk_end(mbez.beziers()[start_data.index], b, single_end_data, end_min);
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
