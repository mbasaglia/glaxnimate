#pragma once

#include <cmath>
#include <QtMath>
#include <QtGlobal>

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

constexpr qreal rad2deg(qreal rad) noexcept
{
    return rad / pi * 180;
}

constexpr qreal deg2rad(qreal rad) noexcept
{
    return rad * pi / 180;
}

template<class Numeric>
Numeric fmod(Numeric x, Numeric y)
{
    return x < 0 ? std::fmod(x, y) + y : std::fmod(x, y);
}


template <typename T>
constexpr inline const T & min(const T &a, const T &b) noexcept { return (a < b) ? a : b; }
template <typename T>
constexpr inline const T & max(const T &a, const T &b) noexcept { return (a < b) ? b : a; }
template <typename T>
constexpr  inline const T &bound(const T &vmin, const T &val, const T &vmax) noexcept
{ return max(vmin, min(vmax, val)); }

} // namespace math
