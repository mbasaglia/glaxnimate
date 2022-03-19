#pragma once

#include <QPointF>

#include "bezier/bezier.hpp"

namespace glaxnimate::math {

class EllipseSolver
{
public:
    /**
     * \param center      2D vector, center of the ellipse
     * \param radii       2D vector, x/y radius of the ellipse
     * \param xrot        Angle between the main axis of the ellipse and the x axis (in radians)
     */
    EllipseSolver(const QPointF& center, const QPointF& radii, qreal xrot);

    QPointF point(qreal t) const;

    QPointF derivative(qreal t) const;

    bezier::Bezier to_bezier(qreal anglestart, qreal angle_delta);

    static bezier::Bezier from_svg_arc(
        QPointF start, qreal rx, qreal ry, qreal xrot,
        bool large, bool sweep, QPointF dest
    );

private:
    static qreal _alpha(qreal step);

    static QPointF _matrix_mul(qreal phi, const QPointF p, qreal sin_mul=1);

    static qreal _angle(const QPointF& u, const QPointF& v);

    QPointF center;
    QPointF radii;
    qreal xrot;
};

} // namespace glaxnimate::math

