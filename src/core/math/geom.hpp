#pragma once

#include <QPointF>
#include "vector.hpp"

namespace math {

/**
 * \brief Finds the closest point to a line
 * \param line_a Point to determine the line
 * \param line_b Second point to determine the line
 * \param p      Point to find the closest of
 * \returns The point on the line that is closest to \p p
 */
inline QPointF line_closest_point(const QPointF& line_a, const QPointF& line_b, const QPointF& p)
{
    QPointF a_to_p = p - line_a;
    QPointF a_to_b = line_b - line_a;

    qreal atb2 = length_squared(a_to_b);
    qreal atp_dot_atb = QPointF::dotProduct(a_to_p, a_to_b);
    qreal t = atp_dot_atb / atb2;

    return line_a + a_to_b * t;
}

} // namespace math
