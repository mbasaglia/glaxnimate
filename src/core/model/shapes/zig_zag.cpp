#include "zig_zag.hpp"
#include "math/geom.hpp"
#include "math/vector.hpp"
#include "math/bezier/bezier_length.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::ZigZag)

using namespace glaxnimate;
using namespace glaxnimate::math::bezier;
using BezierSolver = glaxnimate::math::bezier::CubicBezierSolver<QPointF>;


static double angle_mean(double a, double b)
{
    if ( math::abs(a-b) > math::pi )
        return (a + b) / 2 + math::pi;

    return (a + b) / 2;
}

static void zig_zag_corner(Bezier& output_bezier, const BezierSolver* segment_before, const BezierSolver* segment_after, float amplitude, int direction, float tangent_length)
{
    QPointF point;
    double angle;
    double tan_angle;

    // We use 0.01 and 0.99 instead of 0 and 1 because they yield better results
    if ( !segment_before )
    {
        point = segment_after->points()[0];
        angle = segment_after->normal_angle(0.01);
        tan_angle = segment_after->tangent_angle(0.01);
    }
    else if ( !segment_after )
    {
        point = segment_before->points()[3];
        angle = segment_before->normal_angle(0.99);
        tan_angle = segment_before->tangent_angle(0.99);
    }
    else
    {
        point = segment_after->points()[0];
        angle = -angle_mean(segment_after->normal_angle(0.01), segment_before->normal_angle(0.99));
        tan_angle = angle_mean(segment_after->tangent_angle(0.01), segment_before->tangent_angle(0.99));
    }

    output_bezier.add_point(point + math::from_polar<QPointF>(direction * amplitude, angle));
    auto& vertex = output_bezier.back();

    // It's ok to float-compare as it's a value we set explicitly to 0
    if ( tangent_length != 0 )
    {
        vertex.tan_in = vertex.pos + math::from_polar<QPointF>(-tangent_length, tan_angle);
        vertex.tan_out = vertex.pos + math::from_polar<QPointF>(tangent_length, tan_angle);
    }
}

static int zig_zag_segment(Bezier& output_bezier,const BezierSolver& segment, const LengthData& seg_len, float amplitude, int frequency, int direction, float tangent_length)
{
    for ( int i = 0; i < frequency; i++ )
    {
        auto f = (i + 1.) / (frequency + 1.);
        auto t = seg_len.at_ratio(f).ratio;
        auto angle = segment.normal_angle(t);
        auto point = segment.solve(t);

        output_bezier.add_point(point + math::from_polar<QPointF>(direction * amplitude, -angle));
        auto& vertex = output_bezier.back();

        // It's ok to float-compare as it's a value we set explicitly to 0
        if ( tangent_length != 0 )
        {
            auto tan_angle = segment.tangent_angle(t);
            vertex.tan_in = vertex.pos + math::from_polar<QPointF>(-tangent_length, tan_angle);
            vertex.tan_out = vertex.pos + math::from_polar<QPointF>(tangent_length, tan_angle);
        }

        direction = -direction;
    }

    return direction;
}

static Bezier zig_zag_bezier(const Bezier& input_bezier, float amplitude, int frequency, model::ZigZag::Style style)
{
    Bezier output_bezier;

    output_bezier.set_closed(input_bezier.closed());
    auto count = input_bezier.segment_count();

    if ( count == 0 )
        return output_bezier;

    auto direction = -1;
    BezierSolver segment = input_bezier.segment(count - 1);
    BezierSolver next_segment = input_bezier.segment(0);
    LengthData seg_len(next_segment, 20);

    auto tangent_length = style == model::ZigZag::Wave ? seg_len.length() / (frequency + 1.) / 2. : 0;

    zig_zag_corner(output_bezier, input_bezier.closed() ? &segment : nullptr, &next_segment, amplitude, direction, tangent_length);

    for ( auto i = 0; i < count; i++ )
    {
        segment = next_segment;

        direction = zig_zag_segment(output_bezier, segment, seg_len, amplitude, frequency, -direction, tangent_length);

        if ( i == count - 1 && !input_bezier.closed() )
        {
            zig_zag_corner(output_bezier, &segment, nullptr, amplitude, direction, tangent_length);
        }
        else
        {
            next_segment = input_bezier.segment((i + 1) % count);

            seg_len = LengthData (next_segment, 20);
            zig_zag_corner(output_bezier, &segment, &next_segment, amplitude, direction, tangent_length);
        }
    }

    return output_bezier;
}


QIcon glaxnimate::model::ZigZag::static_tree_icon()
{
    return QIcon::fromTheme("path-simplify");
}

QString glaxnimate::model::ZigZag::static_type_name_human()
{
    return tr("Zig Zag");
}

bool glaxnimate::model::ZigZag::process_collected() const
{
    return false;
}

glaxnimate::math::bezier::MultiBezier glaxnimate::model::ZigZag::process(
    glaxnimate::model::FrameTime t,
    const math::bezier::MultiBezier& mbez
) const
{
    if ( mbez.empty() )
        return {};

    int frequency = math::max(0, qRound(this->frequency.get_at(t)));
    auto amplitude = this->amplitude.get_at(t);
    auto point_type = this->style.get();

    MultiBezier out;

    for ( const auto& inbez : mbez.beziers() )
        out.beziers().push_back(zig_zag_bezier(inbez, amplitude, frequency, point_type));

    return out;
}
