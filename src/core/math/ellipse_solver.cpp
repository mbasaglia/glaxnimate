#include "ellipse_solver.hpp"

#include "bezier/bezier.hpp"
#include "vector.hpp"
#include "math.hpp"


math::EllipseSolver::EllipseSolver(const QPointF& center, const QPointF& radii, qreal xrot)
: center(center),
    radii(radii),
    xrot(xrot)
{}

QPointF math::EllipseSolver::point(qreal t) const
{
    return QPointF(
        center.x()
        + radii.x() * qCos(xrot) * qCos(t)
        - radii.y() * qSin(xrot) * qSin(t),

        center.y()
        + radii.x() * qSin(xrot) * qCos(t)
        + radii.y() * qCos(xrot) * qSin(t)
    );
}

QPointF math::EllipseSolver::derivative(qreal t) const
{
    return QPointF(
        - radii.x() * qCos(xrot) * qSin(t)
        - radii.y() * qSin(xrot) * qCos(t),

        - radii.x() * qSin(xrot) * qSin(t)
        + radii.y() * qCos(xrot) * qCos(t)
    );
}

math::bezier::Bezier math::EllipseSolver::to_bezier(qreal anglestart, qreal angle_delta)
{
    bezier::Bezier points;
    qreal angle1 = anglestart;
    qreal angle_left = qAbs(angle_delta);
    qreal step = math::pi / 2;
    qreal sign = anglestart+angle_delta < angle1 ? -1 : 1;

    // We need to fix the first handle
    qreal firststep = qMin(angle_left, step) * sign;
    qreal alpha = _alpha(firststep);
    QPointF q1 = derivative(angle1) * alpha;
    points.push_back(bezier::Point::from_relative(point(angle1), QPointF(0, 0), q1, math::bezier::Symmetrical));

    // Then we iterate until the angle has been completed
    qreal tolerance = step / 2;
    while ( angle_left > tolerance )
    {
        qreal lstep = qMin(angle_left, step);
        qreal step_sign = lstep * sign;
        qreal angle2 = angle1 + step_sign;
        angle_left -= abs(lstep);

        alpha = _alpha(step_sign);
        QPointF p2 = point(angle2);
        QPointF q2 = derivative(angle2) * alpha;

        points.push_back(bezier::Point::from_relative(p2, -q2, q2, math::bezier::Symmetrical));
        angle1 = angle2;
    }

    if ( points.size() > 1 && qFuzzyCompare(angle_delta, math::tau)  )
    {
        points.close();
        points[0].tan_in = points.back().tan_in;
        points.points().pop_back();
    }
    return points;
}

math::bezier::Bezier math::EllipseSolver::from_svg_arc(
    QPointF start, qreal rx, qreal ry, qreal xrot,
    bool large, bool sweep, QPointF dest
)
{
    rx = qAbs(rx);
    ry = qAbs(ry);

    qreal x1 = start.x();
    qreal y1 = start.y();
    qreal x2 = dest.x();
    qreal y2 = dest.y();
    qreal phi = M_PI * xrot / 180;

    QPointF p1 = _matrix_mul(phi, (start-dest)/2, -1);
    qreal x1p = p1.x();
    qreal y1p = p1.y();

    qreal cr = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry);
    if ( cr > 1 )
    {
        qreal s = qSqrt(cr);
        rx *= s;
        ry *= s;
    }

    qreal dq = rx * rx * y1p * y1p + ry * ry * x1p * x1p;
    qreal pq = (rx * rx * ry * ry - dq) / dq;
    qreal cpm = qSqrt(qMax(0., pq));
    if ( large == sweep )
        cpm = -cpm;
    QPointF cp(cpm * rx * y1p / ry, -cpm * ry * x1p / rx);
    QPointF c = _matrix_mul(phi, cp) + QPointF((x1+x2)/2, (y1+y2)/2);
    qreal theta1 = _angle(QPointF(1, 0), QPointF((x1p - cp.x()) / rx, (y1p - cp.y()) / ry));
    qreal deltatheta = std::fmod(_angle(
        QPointF((x1p - cp.x()) / rx, (y1p - cp.y()) / ry),
        QPointF((-x1p - cp.x()) / rx, (-y1p - cp.y()) / ry)
    ), 2*M_PI);

    if ( !sweep && deltatheta > 0 )
        deltatheta -= 2*M_PI;
    else if ( sweep && deltatheta < 0 )
        deltatheta += 2*M_PI;

    return EllipseSolver(c, QPointF(rx, ry), phi).to_bezier(theta1, deltatheta);
}

qreal math::EllipseSolver::_alpha(qreal step)
{
    return qSin(step) * (qSqrt(4+3*qPow(qTan(step/2), 2)) - 1) / 3;
}

QPointF math::EllipseSolver::_matrix_mul(qreal phi, const QPointF p, qreal sin_mul)
{
    qreal c = qCos(phi);
    qreal  s = qSin(phi) * sin_mul;

    qreal xr = c * p.x() - s * p.y();
    qreal yr = s * p.x() + c * p.y();
    return QPointF(xr, yr);

}
qreal math::EllipseSolver::_angle(const QPointF& u, const QPointF& v)
{
    qreal arg = qAcos(qMax(-1., qMin(1., QPointF::dotProduct(u, v) / (length(u) * length(v)))));
    if ( u.x() * v.y() - u.y() * v.x() < 0 )
        return -arg;
    return arg;
}


