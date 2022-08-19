#pragma once

#include <cmath>
#include <QtMath>
#include <QtGlobal>

namespace glaxnimate::math {

constexpr const qreal pi = 3.14159265358979323846;
constexpr const qreal tau = pi*2;
constexpr const qreal sqrt_2 = M_SQRT2;

using std::sqrt;
using std::sin;
using std::cos;
using std::tan;
using std::acos;
using std::asin;
using std::atan;
using std::atan2;
using std::pow;

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
    return x < 0 ?
        std::fmod(std::fmod(x, y) + y, y) :
        std::fmod(x, y)
    ;
}


template <typename T>
constexpr inline const T & min(const T &a, const T &b) noexcept { return (a < b) ? a : b; }
template <typename T>
constexpr inline const T & max(const T &a, const T &b) noexcept { return (a < b) ? b : a; }
template <typename T>
constexpr  inline const T &bound(const T &vmin, const T &val, const T &vmax) noexcept
{ return max(vmin, min(vmax, val)); }

template<class T> constexpr inline T abs(T t) noexcept { return t < 0 ? -t : t; }

/**
 * \brief Reverses linear interpolation
 * \param a First value interpolated from
 * \param b Second value interpolated from
 * \param c Interpolation result
 * \pre a < b && a <= c <= b
 * \returns Factor \p f so that lerp(a, b, f) == c
 */
template<class T> constexpr qreal unlerp(const T& a, const T& b, const T& c)
{
    return qreal(c-a) / qreal(b-a);
}

} // namespace glaxnimate::math
