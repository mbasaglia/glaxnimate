#pragma once
#include <utility>
#include "bezier.hpp"

namespace math::bezier {

/**
 * \brief Turns all points in the curve to smooth, sutomatically setting tangents
 */
void auto_smooth(Bezier& curve, int start, int end);

/**
 * \brief Reduces the number of points in a bezier curve
 */
void simplify(Bezier& curve, qreal threshold);


struct ProjectResult
{
    int index = 0;
    qreal factor = 0;
    qreal distance = std::numeric_limits<qreal>::max();
    QPointF point = {};
};

ProjectResult project(const Bezier& curve, const QPointF& p);

} // namespace math
