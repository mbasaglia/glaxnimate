#pragma once
#include "bezier.hpp"

namespace math::bezier {

/**
 * \brief Turns all points in the curve to smooth, sutomatically setting tangents
 */
void auto_smooth(Bezier& curve);

/**
 * \brief Reduces the number of points in a bezier curve
 */
void simplify(Bezier& curve, qreal threshold);

} // namespace math
