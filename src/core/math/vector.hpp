#pragma once

#include <tuple>
#include <utility>
#include <cmath>
#include <QMetaType>
#include <QColor>


class QVector2D; class QVector3D; class QVector4D; class QPointF;

namespace glaxnimate::math {


namespace detail {

template<class V> struct VecSize;
template<> struct VecSize<QVector2D> { static constexpr int value = 2; };
template<> struct VecSize<QVector3D> { static constexpr int value = 3; };
template<> struct VecSize<QVector4D> { static constexpr int value = 4; };
template<> struct VecSize<QPointF> { static constexpr int value = 2; };

template<class VecT> struct VecScalar
{
    using type = std::decay_t<decltype(std::declval<VecT>()[0])>;
};


template<> struct VecScalar<QPointF> { using type = qreal; };
template<> struct VecScalar<QSizeF> { using type = qreal; };

template<class VecT>
using scalar_type = typename detail::VecScalar<std::decay_t<VecT>>::type;

template<class VecT>
constexpr const scalar_type<VecT>& get(const VecT& vt, int off) noexcept
{
    return reinterpret_cast<const typename VecScalar<VecT>::type*>(&vt)[off];
}

template<class VecT>
constexpr scalar_type<VecT>& get(VecT& vt, int off) noexcept
{
    return reinterpret_cast<typename VecScalar<VecT>::type*>(&vt)[off];
}

template<class VecT, int d>
struct LengthHelper
{
    static constexpr scalar_type<VecT> sumsq(const VecT& v) noexcept
    {
        return get(v, d-1) * get(v, d-1) + LengthHelper<VecT, d-1>::sumsq(v);
    }
};

template<class VecT>
struct LengthHelper<VecT, 1>
{
    static constexpr scalar_type<VecT> sumsq(const VecT& v) noexcept
    {
        return get(v, 0) * get(v, 0);
    }
};


} // namespace detail

using detail::scalar_type;
using detail::get;

template<class T>
constexpr T lerp(const T& a, const T& b, double factor)
{
    return a * (1-factor) + b * factor;
}

inline QColor lerp(const QColor& a, const QColor& b, double factor)
{
    return QColor::fromRgbF(
        lerp(a.redF(), b.redF(), factor),
        lerp(a.greenF(), b.greenF(), factor),
        lerp(a.blueF(), b.blueF(), factor),
        lerp(a.alphaF(), b.alphaF(), factor)
    );
}

template<class T>
constexpr std::vector<T> lerp(const std::vector<T>& a, const std::vector<T>& b, double factor)
{
    if ( a.size() != b.size() )
        return a;
    std::vector<T> c;
    c.reserve(a.size());
    for ( std::size_t i = 0; i < a.size(); i++ )
        c.push_back(lerp(a[i], b[i], factor));
    return c;
}

template<class VecT>
constexpr scalar_type<VecT> length_squared(const VecT& v)
{
    return detail::LengthHelper<VecT, detail::VecSize<VecT>::value>::sumsq(v);
}

/**
 * \brief 2-norm length of a vector
 */
template<class VecT>
constexpr scalar_type<VecT> length(const VecT& v)
{
    return std::sqrt(length_squared(v));
}


/**
 * \brief 2-norm length of vector difference
 */
template<class VecT>
constexpr scalar_type<VecT> distance(const VecT& a, const VecT& b)
{
    return length(a - b);
}

/**
 * \brief Angle to the x axis of a 2D cartesian vector
 */
template<class VecT>
scalar_type<VecT> angle(const VecT& cartesian)
{
    return std::atan2(detail::get(cartesian, 1), detail::get(cartesian, 0));
}

template<class VecT>
VecT from_polar(scalar_type<VecT> length, scalar_type<VecT> angle)
{
    return {std::cos(angle) * length, std::sin(angle) * length};
}

template<class VecT>
struct PolarVector
{
    using scalar = scalar_type<VecT>;

    scalar length;
    scalar angle;

    constexpr PolarVector() noexcept
    : PolarVector(0, 0)
    {}

    constexpr PolarVector(scalar length, scalar angle) noexcept
    : length(length), angle(angle)
    {}

    PolarVector(const VecT& cartesian)
    : length(math::length(cartesian)),
      angle(math::angle(cartesian))
    {}

    VecT to_cartesian() const
    {
        return {std::cos(angle) * length, std::sin(angle) * length};
    }
};

template<class VecT>
bool fuzzy_compare(const VecT& a, const VecT& b)
{
    for ( int i = 0; i < detail::VecSize<VecT>::value; i++ )
        if ( !qFuzzyCompare(detail::get(a, i), detail::get(b, i)) )
            return false;
    return true;
}

} // namespace glaxnimate::math
