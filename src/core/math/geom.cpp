#include "geom.hpp"
#include <array>

using namespace glaxnimate;


QPointF math::line_closest_point(const QPointF& line_a, const QPointF& line_b, const QPointF& p)
{
    QPointF a_to_p = p - line_a;
    QPointF a_to_b = line_b - line_a;

    qreal atb2 = length_squared(a_to_b);
    qreal atp_dot_atb = QPointF::dotProduct(a_to_p, a_to_b);
    qreal t = atp_dot_atb / atb2;

    return line_a + a_to_b * t;
}

// Algorithm from http://ambrsoft.com/TrigoCalc/Circle3D.htm
QPointF math::circle_center(const QPointF& p1, const QPointF& p2, const QPointF& p3)
{
    qreal x1 = p1.x();
    qreal x2 = p2.x();
    qreal x3 = p3.x();
    qreal y1 = p1.y();
    qreal y2 = p2.y();
    qreal y3 = p3.y();
    qreal A = 2 * (x1 * (y2 - y3) - y1 * (x2 - x3) + x2 * y3 - x3 * y2);
    qreal p12 = x1*x1 + y1*y1;
    qreal p22 = x2*x2 + y2*y2;
    qreal p32 = x3*x3 + y3*y3;
    qreal B = p12 * (y3 - y2) + p22 * (y1 - y3) + p32 * (y2 - y1);
    qreal C = p12 * (x2 - x3) + p22 * (x3 - x1) + p32 * (x1 - x2);

    return {
        - B / A,
        - C / A
    };
}

// Custom implementation rather than using QVector3D to keep precision
static std::array<qreal, 3> cross_product(const std::array<qreal, 3>& a, const std::array<qreal, 3>& b)
{
    return {
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    };
}

std::optional<QPointF> math::line_intersection(const QPointF& start1, const QPointF& end1, const QPointF& start2, const QPointF& end2)
{
    std::array<qreal, 3> v1{start1.x(), start1.y(), 1};
    std::array<qreal, 3> v2{end1.x(), end1.y(), 1};
    std::array<qreal, 3> v3{start2.x(), start2.y(), 1};
    std::array<qreal, 3> v4{end2.x(), end2.y(), 1};

    std::array<qreal, 3> cp = cross_product(
        cross_product(v1, v2),
        cross_product(v3, v4)
    );

    // More forgiving than qFuzzyIsNull
    if ( qAbs(cp[2]) <= 0.00001 )
        return {};

    return QPointF(cp[0] / cp[2], cp[1] / cp[2]);
}
