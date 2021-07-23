#pragma once

#include "math/bezier/bezier.hpp"

namespace glaxnimate::math {

bezier::MultiBezier voronoi(std::vector<QPointF> points, const QRectF& bounds);

} // namespace glaxnimate::math
