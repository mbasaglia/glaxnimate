#include "offset_path.hpp"
#include "math/geom.hpp"
#include "math/vector.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::OffsetPath)

using namespace glaxnimate;
using namespace glaxnimate::math::bezier;

static bool point_fuzzy_compare(const QPointF& a, const QPointF& b)
{
    return qFuzzyCompare(a.x(), b.x()) && qFuzzyCompare(a.y(), b.y());
}

/*
    Simple offset of a linear segment
*/
static std::pair<QPointF, QPointF> linear_offset(const QPointF& p1, const QPointF& p2, float amount)
{
    auto angle = math::atan2(p2.x() - p1.x(), p2.y() - p1.y());
    auto offset = math::from_polar<QPointF>(amount, -angle);
    return {
        p1 + offset,
        p2 + offset
    };
}

/*
    Offset a bezier segment
    only works well if the segment is flat enough
*/
static math::bezier::CubicBezierSolver<QPointF> offset_segment(const math::bezier::CubicBezierSolver<QPointF>& segment, float amount)
{
    QPointF p0, p1a, p1b, p2b, p2a, p3;
    std::tie(p0, p1a) = linear_offset(segment.points()[0], segment.points()[1], amount);
    std::tie(p1b, p2b) = linear_offset(segment.points()[1], segment.points()[2], amount);
    std::tie(p2a, p3) = linear_offset(segment.points()[2], segment.points()[3], amount);
    QPointF p1 = math::line_intersection(p0, p1a, p1b, p2b).value_or(p1a);
    QPointF p2 = math::line_intersection(p2a, p3, p1b, p2b).value_or(p2a);

    return {p0, p1, p2, p3};
}

/*
    Join two segments
*/
static QPointF join_lines(
    Bezier& output_bezier,
    const CubicBezierSolver<QPointF>& seg1,
    const CubicBezierSolver<QPointF>& seg2,
    glaxnimate::model::Stroke::Join line_join,
    float miter_limit
)
{
    QPointF p0 = seg1.points()[3];
    QPointF p1 = seg2.points()[0];

   if ( line_join == glaxnimate::model::Stroke::BevelJoin )
        return p0;

    // Connected, they don't need a joint
    if ( point_fuzzy_compare(p0, p1) )
        return p0;

    auto& last_point = output_bezier.points().back();

    if ( line_join == glaxnimate::model::Stroke::RoundJoin )
    {
        auto angle_out = seg1.tangent_angle(1);
        auto angle_in = seg2.tangent_angle(0) + math::pi;
        auto offset = math::from_polar<QPointF>(100, angle_out + math::pi / 2);
        auto center = math::line_intersection(p0, p0 + offset, p1, p1 + offset);
        auto radius = center ? math::distance(*center, p0) : math::distance(p0, p1) / 2;
        last_point.tan_out = last_point.pos +
            math::from_polar<QPointF>(2 * radius * math::ellipse_bezier, angle_out);

        output_bezier.add_point(p1, math::from_polar<QPointF>(2 * radius * math::ellipse_bezier, angle_in));

        return p1;
    }

    // Miter
    auto t0 = point_fuzzy_compare(p0, seg1.points()[2]) ? seg1.points()[0] : seg1.points()[2];
    auto t1 = point_fuzzy_compare(p1, seg2.points()[1]) ? seg2.points()[3] : seg2.points()[1];
    auto intersection = math::line_intersection(t0, p0, p1, t1);

    if ( intersection && math::distance(*intersection, p0) < miter_limit )
    {
        output_bezier.add_point(*intersection);
        return *intersection;
    }

    return p0;
}


static std::optional<std::pair<float, float>> get_intersection(
    const CubicBezierSolver<QPointF>&a,
    const CubicBezierSolver<QPointF>& b)
{
    auto intersect = a.intersections(b, 2, 3, 7);

    std::size_t i = 0;
    if ( !intersect.empty() && qFuzzyCompare(intersect[0].first, 1) )
        i++;

    if ( intersect.size() > i )
        return intersect[i];

    return {};
}

static std::pair<std::vector<CubicBezierSolver<QPointF>>, std::vector<CubicBezierSolver<QPointF>>>
prune_segment_intersection(
    const std::vector<CubicBezierSolver<QPointF>>& a,
    const std::vector<CubicBezierSolver<QPointF>>& b
)
{
    auto out_a = a;
    auto out_b = b;

    auto intersect = get_intersection(a.back(), b[0]);

    if ( intersect )
    {
        out_a.back() = a.back().split(intersect->first).first;
        out_b[0] = b[0].split(intersect->second).second;
    }

    if ( a.size() > 1 && b.size() > 1 )
    {
        intersect = get_intersection(a[0], b.back());

        if ( intersect )
        {
            return {
                {a[0].split(intersect->first).first},
                {b.back().split(intersect->second).second},
            };
        }
    }

    return {out_a, out_b};
}

void prune_intersections(std::vector<std::vector<CubicBezierSolver<QPointF>>>& segments)
{
    for ( std::size_t i = 1; i < segments.size() ; i++ )
    {
        std::tie(segments[i-1], segments[i]) = prune_segment_intersection(segments[i - 1], segments[i]);
    }

    if ( segments.size() > 1 )
        std::tie(segments.back(), segments[0]) = prune_segment_intersection(segments.back(), segments[0]);

}

std::vector<math::bezier::CubicBezierSolver<QPointF>> offset_segment_split(
    const math::bezier::CubicBezierSolver<QPointF>& segment,
    float amount
)
{
    /*
        We split each bezier segment into smaller pieces based
        on inflection points, this ensures the control point
        polygon is convex.

        (A cubic bezier can have none, one, or two inflection points)
    */
    auto flex = segment.inflection_points();

    if ( flex.size() == 0 )
    {
        return {offset_segment(segment, amount)};
    }
    else if ( flex.size() == 1 || flex[1] == 1 )
    {
        auto split = segment.split(flex[0]);

        return {
            offset_segment(split.first, amount),
            offset_segment(split.second, amount)
        };
    }
    else
    {
        auto split_1 = segment.split(flex[0]);
        float t = (flex[1] - flex[0]) / (1 - flex[0]);
        auto split_2 = CubicBezierSolver(split_1.second).split(t);

        return {
            offset_segment(split_1.first, amount),
            offset_segment(split_2.first, amount),
            offset_segment(split_2.second, amount)
        };
    }
}

static MultiBezier offset_path(
    // Beziers as collected from the other shapes
    const MultiBezier& collected_shapes,
    float amount,
    model::Stroke::Join line_join,
    float miter_limit
)
{
    MultiBezier result;

    for ( const auto& input_bezier : collected_shapes.beziers() )
    {
        int count = input_bezier.segment_count();
        Bezier output_bezier;

        output_bezier.set_closed(input_bezier.closed());

        std::vector<std::vector<math::bezier::CubicBezierSolver<QPointF>>> multi_segments;
        for ( int i = 0; i < count; i++ )
            multi_segments.push_back(offset_segment_split(input_bezier.segment(i), amount));

        // Open paths are stroked rather than being simply offset
        if ( !input_bezier.closed() )
        {
            for ( int i = count - 1; i >= 0; i-- )
                multi_segments.push_back(offset_segment_split(input_bezier.inverted_segment(i), amount));
        }

        prune_intersections(multi_segments);

        // Add bezier segments to the output and apply line joints
        QPointF last_point;
        const math::bezier::CubicBezierSolver<QPointF>* last_seg = nullptr;

        for ( const auto& multi_segment : multi_segments )
        {
            if ( last_seg )
                last_point = join_lines(output_bezier, *last_seg, multi_segment[0], line_join, miter_limit);

            last_seg = &multi_segment.back();

            for ( const auto& segment : multi_segment )
            {
                if ( !point_fuzzy_compare(segment.points()[0], last_point) || output_bezier.empty() )
                    output_bezier.add_point(segment.points()[0]);

                output_bezier.back().tan_out = segment.points()[1];


                output_bezier.add_point(segment.points()[3]);
                output_bezier.back().tan_in = segment.points()[2];

                last_point = segment.points()[3];
            }
        }

        if ( !multi_segments.empty() )
            join_lines(output_bezier, *last_seg, multi_segments[0][0], line_join, miter_limit);

        result.beziers().push_back(output_bezier);
    }

    return result;
}

QIcon glaxnimate::model::OffsetPath::static_tree_icon()
{
    return QIcon::fromTheme("path-offset-dynamic");
}

QString glaxnimate::model::OffsetPath::static_type_name_human()
{
    return tr("Offset Path");
}

bool glaxnimate::model::OffsetPath::process_collected() const
{
    return false;
}

glaxnimate::math::bezier::MultiBezier glaxnimate::model::OffsetPath::process(
    glaxnimate::model::FrameTime t,
    const math::bezier::MultiBezier& mbez
) const
{
    if ( mbez.empty() )
        return {};

    auto amount = this->amount.get_at(t);

    if ( qFuzzyIsNull(amount) )
        return mbez;

    return offset_path(mbez, amount, join.get(), miter_limit.get_at(t));
}
