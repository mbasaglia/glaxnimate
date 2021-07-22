#include "cubic_struts.hpp"

#include "math/math.hpp"
#include "math/geom.hpp"

#include "operations.hpp"

using namespace glaxnimate;

// see https://pomax.github.io/bezierinfo/#abc (this returns A given B)
QPointF math::bezier::get_quadratic_handle(const math::bezier::BezierSegment& segment, const QPointF& B, qreal t)
{
    qreal t1 = (1-t);
    qreal t13 = t1 * t1 * t1;
    qreal t3 = t * t * t;
    qreal u = t13 / (t3 + t13);
    qreal ratio = math::abs((t3 + t13 - 1) / (t3 + t13));
    QPointF C = math::lerp(segment[3], segment[0], u);
    QPointF A;
    if ( t == 0 )
        A = segment[1];
    else if ( t == 1 )
        A = segment[2];
    else
        A = B + ( B - C ) / ratio;

    return A;
}


math::bezier::BezierSegment math::bezier::cubic_segment_from_struts(
    const math::bezier::BezierSegment& segment,
    const BezierStruts& struts
)
{
    if ( struts.t == 0  || struts.t == 1)
        return segment;

    QPointF A = get_quadratic_handle(segment, struts.B, struts.t);
    QPointF v1 = A + (struts.e1 - A) / (1-struts.t);
    QPointF v2 = A + (struts.e2 - A) / struts.t;
    return {
        segment[0],
        segment[0] + (v1 - segment[0]) / struts.t,
        segment[3] + (v2 - segment[3]) / (1-struts.t),
        segment[3]
    };
}

// see https://pomax.github.io/bezierinfo/#pointcurves
math::bezier::BezierStruts math::bezier::cubic_struts_idealized(const math::bezier::BezierSegment& segment, const QPointF& B)
{
    BezierStruts struts;
    struts.B = B;
    qreal d1 = math::length(segment[0] - B);
    qreal d2 = math::length(segment[3] - B);
    struts.t = d1 / (d1+d2);
    QPointF center = circle_center(segment[0], B, segment[3]);
    qreal tanlen  = math::length(segment[3] - segment[0]) / 3;
    qreal phi = math::fmod(
        math::atan2(segment[3].y() - segment[0].y(), segment[3].x() - segment[0].x())
        - math::atan2(B.y() - segment[0].y(), B.x() - segment[0].x())
        + math::tau,
        math::tau
    );
    if ( phi < math::pi )
        tanlen = -tanlen;

    qreal de1 = struts.t * tanlen;
    qreal de2 = (1-struts.t) * tanlen;
    QPointF tangent = struts.B - center;
    tangent /= math::length(tangent);
    tangent = { -tangent.y(), tangent.x() };
    struts.e1 = struts.B + de1 * tangent;
    struts.e2 = struts.B - de2 * tangent;

    return struts;
}

math::bezier::BezierStruts math::bezier::cubic_struts_projection(
    const math::bezier::BezierSegment& segment,
    const QPointF& B,
    const math::bezier::ProjectResult& projection
)
{
    BezierStruts struts;
    struts.B = B;
    struts.t = projection.factor;

    auto v1 = math::lerp(segment[0], segment[1], struts.t);
    auto v2 = math::lerp(segment[2], segment[3], struts.t);
    auto A = get_quadratic_handle(segment, projection.point, struts.t);
    struts.e1 = math::lerp(v1, A, struts.t) - projection.point + B;
    struts.e2 = math::lerp(A, v2, struts.t) - projection.point + B;

    return struts;
}
