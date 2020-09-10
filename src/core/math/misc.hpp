#pragma once

#include <QtMath>

namespace math {

constexpr const qreal pi = M_PI;
constexpr const qreal tau = M_PI*2;
constexpr const qreal sqrt_2 = M_SQRT2;

using std::sqrt;
using std::sin;
using std::cos;
using std::tan;
using std::acos;
using std::asin;
using std::atan;
using std::atan2;

inline qreal hypot(qreal x, qreal y)
{
    return sqrt(x*x + y*y);
}

template<class Numeric>
constexpr Numeric sign(Numeric x) noexcept
{
    return x < 0 ? -1 : 1;
}


} // namespace math
