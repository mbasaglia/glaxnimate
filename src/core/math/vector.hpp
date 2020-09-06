#pragma once

#include <tuple>
#include <utility>
#include <cmath>
#include <QMetaType>
#include <QColor>


class QVector2D; class QVector3D; class QVector4D; class QPointF;

namespace math {


namespace detail {

template<class Derived, class ScalarT, int Size> class VecN;

template<class Derived, class ScalarT, int Size, int I>
struct VecGetter
{
    static_assert(Size >= I && Size > 0);

    using VecCur = VecN<Derived, ScalarT, Size>;
    using VecSmol = VecN<Derived, ScalarT, Size-1>;
    using Smol = VecGetter<Derived, ScalarT, Size-1, I>;


    static constexpr ScalarT get(const VecCur& v) noexcept {
        return Smol::get(static_cast<const VecSmol&>(v));
    }

    static constexpr ScalarT& get_ref(VecCur& v) noexcept {
        return Smol::get_ref(static_cast<VecSmol&>(v));
    }

    template<class Func>
    static constexpr void unary(VecCur& v, Func&& func) noexcept
    {
        func(v.v_);
        return Smol::unary(static_cast<VecSmol&>(v), std::forward<Func>(func));
    }

    template<class Func>
    static constexpr void binary(VecCur& a, const VecCur& b, Func&& func) noexcept
    {
        func(a.v_, b.v_);
        return Smol::binary(static_cast<VecSmol&>(a), static_cast<const VecSmol&>(b), std::forward<Func>(func));
    }

    static constexpr bool equal(const VecCur& a, const VecCur& b) noexcept
    {
        return a.v_ == b.v_ && Smol::equal(static_cast<const VecSmol&>(a), static_cast<const VecSmol&>(b));
    }

    static constexpr ScalarT length(const VecCur& a) noexcept
    {
        return a.v_ * a.v_ + Smol::length(static_cast<const VecSmol&>(a));
    }

};

template<class Derived, class ScalarT, int Size>
struct VecGetter<Derived, ScalarT, Size, Size>
{
    using VecCur = VecN<Derived, ScalarT, Size>;

    static constexpr ScalarT get(const VecCur& v) noexcept {
        return v.v_;
    }

    static constexpr ScalarT& get_ref(VecCur& v) noexcept {
        return v.v_;
    }

    template<class Func>
    static constexpr void unary(VecCur&, Func&&) noexcept
    {}

    template<class Func>
    static constexpr void binary(VecCur&, const VecCur&, Func&&) noexcept
    {}

    static constexpr bool equal(const VecCur&, const VecCur&) noexcept { return true; }

    static constexpr ScalarT length(const VecCur&) noexcept { return ScalarT{}; }
};



template<class Derived, class ScalarT, int Size>
class VecN : public VecN<Derived, ScalarT, Size-1>
{
    static_assert(Size > 0);
public:
    using scalar = ScalarT;
    using size_type = int;
    using iterator = scalar*;
    using const_iterator = const scalar*;
    static constexpr const size_type size_static = Size;

    constexpr VecN() noexcept : VecN<Derived, scalar, Size-1>(), v_(0) {}

    template<class... T, class = std::enable_if_t<sizeof...(T) >= Size>>
    constexpr VecN(T... vals) noexcept : VecN(std::forward_as_tuple(vals...)) {}


    template<class... T>
    constexpr VecN(const std::tuple<T...>& tup) noexcept :
        VecN<Derived, scalar, Size-1>(tup),
        v_(std::get<Size-1>(tup))
    {
        static_assert(sizeof...(T) >= Size, "Too few arguments to VecN()");
    }

    constexpr size_type size() const noexcept { return Size; }

    template<size_type i>
    constexpr scalar get() const noexcept
    {
        static_assert(i >= 0 && i < Size, "Invalid index");
        return detail::VecGetter<Derived, scalar, Size, i+1>::get(*this);
    }

    template<size_type i>
    constexpr scalar& get() noexcept
    {
        static_assert(i >= 0 && i < Size, "Invalid index");
        return detail::VecGetter<Derived, scalar, Size, i+1>::get_ref(*this);
    }

    constexpr const scalar* data() const noexcept
    {
        return reinterpret_cast<const scalar*>(this);
    }

    constexpr scalar* data() noexcept
    {
        return reinterpret_cast<scalar*>(this);
    }

    constexpr scalar operator[](size_type i) const noexcept
    {
        return data()[i];
    }

    constexpr scalar& operator[](size_type i) noexcept
    {
        return data()[i];
    }

    constexpr const_iterator begin() const noexcept
    {
        return data();
    }

    constexpr const_iterator end() const noexcept
    {
        return data() + Size;
    }

    constexpr const_iterator cbegin() const noexcept
    {
        return begin();
    }

    constexpr const_iterator cend() const noexcept
    {
        return end();
    }

    constexpr iterator begin() noexcept
    {
        return data();
    }

    constexpr iterator end() noexcept
    {
        return data() + Size;
    }

    constexpr Derived operator-() const noexcept
    {
        Derived copy = d();
        detail::VecGetter<Derived, scalar, Size, 0>::unary(copy, [](scalar& i) constexpr noexcept { i = -i; });
        return copy;
    }

    constexpr Derived& operator+=(const VecN& o) noexcept
    {
        detail::VecGetter<Derived, scalar, Size, 0>::binary(*this, o,
            [](scalar& a, scalar b) constexpr noexcept { a += b; }
        );

        return d();
    }

    constexpr Derived operator+(const VecN& o) const noexcept
    {
        Derived c = d();
        return c += o;
    }

    constexpr Derived& operator-=(const VecN& o) noexcept
    {
        detail::VecGetter<Derived, scalar, Size, 0>::binary(*this, o,
            [](scalar& a, scalar b) constexpr noexcept { a -= b; }
        );

        return d();
    }

    constexpr Derived operator-(const VecN& o) const noexcept
    {
        Derived c = d();
        return c -= o;
    }

    constexpr Derived& operator*=(scalar o) noexcept
    {
        detail::VecGetter<Derived, scalar, Size, 0>::unary(*this,
            [o](scalar& a) constexpr noexcept { a *= o; }
        );
        return d();
    }

    constexpr Derived operator*(scalar o) const noexcept
    {
        Derived c = d();
        return c *= o;
    }

    constexpr Derived& operator/=(scalar o) noexcept
    {
        detail::VecGetter<Derived, scalar, Size, 0>::unary(*this,
            [o](scalar& a) constexpr noexcept { a /= o; }
        );
        return d();
    }

    constexpr Derived operator/(scalar o) const noexcept
    {
        Derived c = d();
        return c /= o;
    }

    constexpr bool operator==(const VecN& o) const noexcept
    {
        return detail::VecGetter<Derived, scalar, Size, 0>::equal(*this, o);
    }


    constexpr bool operator!=(const VecN& o) const noexcept
    {
        return !(*this == o);
    }

    constexpr Derived lerp(const VecN& other, scalar factor) const noexcept
    {
        return *this * (1-factor) + other * factor;
    }

    constexpr ScalarT length_squared() const noexcept
    {
        return detail::VecGetter<Derived, scalar, Size, 0>::length(*this);
    }

    ScalarT length() const noexcept
    {
        return std::sqrt(length_squared());
    }

    Derived& normalize() noexcept
    {
        return *this /= length();
    }

    Derived normalized() const noexcept
    {
        return *this / length();
    }

protected:
    using Ctor = VecN;

private:
    scalar v_;

    template<class, class, int, int> friend struct detail::VecGetter;

    constexpr Derived& d() noexcept { return *static_cast<Derived*>(this); }
    constexpr const Derived& d() const noexcept { return *static_cast<const Derived*>(this); }
    static constexpr Derived& d(VecN& b) noexcept { return static_cast<Derived&>(b); }
    static constexpr const Derived& d(const VecN& b) noexcept { return static_cast<const Derived&>(b); }
};

template<class Derived, class ScalarT> class VecN<Derived, ScalarT, 0>
{
public:
    constexpr VecN() noexcept {}
    template<class... T> constexpr VecN(const std::tuple<T...>&) noexcept {}
};

} // namespace detail

template<class ScalarT, int Size>
class VecN : public detail::VecN<VecN<ScalarT, Size>, ScalarT, Size>
{
public:
    using detail::VecN<VecN<ScalarT, Size>, ScalarT, Size>::VecN;
};

class Vec2 : public detail::VecN<Vec2, double, 2>
{
    Q_GADGET
public:
    using Ctor::Ctor;

    constexpr scalar x() const noexcept { return get<0>(); }
    constexpr scalar& x() noexcept { return get<0>(); }
    constexpr void set_x(scalar v) noexcept { get<0>() = v; }

    constexpr scalar y() const noexcept { return get<1>(); }
    constexpr scalar& y() noexcept { return get<1>(); }
    constexpr void set_y(scalar v) noexcept { get<1>() = v; }
};

class Vec3 : public detail::VecN<Vec3, double, 3>
{
    Q_GADGET
public:
    using Ctor::Ctor;

    constexpr scalar x() const noexcept { return get<0>(); }
    constexpr scalar& x() noexcept { return get<0>(); }
    constexpr void set_x(scalar v) noexcept { get<0>() = v; }

    constexpr scalar y() const noexcept { return get<1>(); }
    constexpr scalar& y() noexcept { return get<1>(); }
    constexpr void set_y(scalar v) noexcept { get<1>() = v; }

    constexpr scalar z() const noexcept { return get<2>(); }
    constexpr scalar& z() noexcept { return get<2>(); }
    constexpr void set_z(scalar v) noexcept { get<2>() = v; }
};

class Vec4 : public detail::VecN<Vec4, double, 4>
{
    Q_GADGET
public:
    using Ctor::Ctor;

    constexpr scalar x() const noexcept { return get<0>(); }
    constexpr scalar& x() noexcept { return get<0>(); }
    constexpr void set_x(scalar v) noexcept { get<0>() = v; }

    constexpr scalar y() const noexcept { return get<1>(); }
    constexpr scalar& y() noexcept { return get<1>(); }
    constexpr void set_y(scalar v) noexcept { get<1>() = v; }

    constexpr scalar z() const noexcept { return get<2>(); }
    constexpr scalar& z() noexcept { return get<2>(); }
    constexpr void set_z(scalar v) noexcept { get<2>() = v; }

    constexpr scalar w() const noexcept { return get<3>(); }
    constexpr scalar& w() noexcept { return get<3>(); }
    constexpr void set_w(scalar v) noexcept { get<3>() = v; }
};



namespace detail {

template<class V> struct VecSize { static constexpr int value = V::size_static; };
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

template<class ScalarT, int Size>
constexpr VecN<ScalarT, Size> lerp(const VecN<ScalarT, Size>& a, const VecN<ScalarT, Size>& b, double factor) noexcept
{
    return a.lerp(b, factor);
}

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
 * \brief Angle to the x axis of a 2D cartesian vector
 */
template<class VecT>
scalar_type<VecT> angle(const VecT& cartesian)
{
    return std::atan2(detail::get(cartesian, 1), detail::get(cartesian, 0));
}

template<class VecT>
struct PolarVector
{
    using scalar = scalar_type<VecT>;

    scalar length;
    scalar angle;

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

} // namespace math
