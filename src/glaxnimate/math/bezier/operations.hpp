#pragma once
#include <utility>
#include "bezier.hpp"

namespace glaxnimate::math::bezier {

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

/**
 * \brief Projects a point onto a bezier curve
 * \param curve The target bezier
 * \param p     The point to project
 * \returns ProjectResult with the point on \p curve closest to \p p
 */
ProjectResult project(const Bezier& curve, const QPointF& p);

ProjectResult project(const BezierSegment& segment, const QPointF& p);

} // namespace glaxnimate::math
