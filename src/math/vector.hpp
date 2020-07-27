#pragma once

#include <tuple>
#include <utility>
#include <cmath>
#include <QMetaType>

namespace math {

template<class ScalarT, int Size> class VecN;

namespace detail {

template<class ScalarT, int Size, int I>
struct VecGetter
{
    static_assert(Size >= I && Size > 0);

    using VecCur = VecN<ScalarT, Size>;
    using VecSmol = VecN<ScalarT, Size-1>;
    using Smol = VecGetter<ScalarT, Size-1, I>;


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

template<class ScalarT, int Size>
struct VecGetter<ScalarT, Size, Size>
{
    using VecCur = VecN<ScalarT, Size>;

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



} // namespace detail

template<class ScalarT, int Size>
class VecN : public VecN<ScalarT, Size-1>
{
    static_assert(Size > 0);
public:
    using scalar = ScalarT;
    using size_type = int;
    using iterator = scalar*;
    using const_iterator = const scalar*;
    static constexpr const size_type size_static = Size;

    constexpr VecN() noexcept : VecN<scalar, Size-1>(), v_(0) {}

    template<class... T>
    constexpr VecN(T... vals) noexcept : VecN(std::forward_as_tuple(vals...)) {}


    template<class... T>
    constexpr VecN(const std::tuple<T...>& tup) noexcept :
        VecN<scalar, Size-1>(tup),
        v_(std::get<Size-1>(tup))
    {
        static_assert(sizeof...(T) >= Size, "Too few arguments to VecN()");
    }

    constexpr size_type size() const noexcept { return Size; }

    template<size_type i>
    constexpr scalar get() const noexcept
    {
        static_assert(i >= 0 && i < Size, "Invalid index");
        return detail::VecGetter<scalar, Size, i+1>::get(*this);
    }

    template<size_type i>
    constexpr scalar& get() noexcept
    {
        static_assert(i >= 0 && i < Size, "Invalid index");
        return detail::VecGetter<scalar, Size, i+1>::get_ref(*this);
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

    constexpr VecN operator-() const noexcept
    {
        VecN copy = *this;
        detail::VecGetter<scalar, Size, 0>::unary(copy, [](scalar& i) constexpr noexcept { i = -i; });
        return copy;
    }

    constexpr VecN& operator+=(const VecN& o) noexcept
    {
        detail::VecGetter<scalar, Size, 0>::binary(*this, o,
            [](scalar& a, scalar b) constexpr noexcept { a += b; }
        );

        return *this;
    }

    constexpr VecN operator+(const VecN& o) const noexcept
    {
        VecN c = *this;
        return c += o;
    }

    constexpr VecN& operator-=(const VecN& o) noexcept
    {
        detail::VecGetter<scalar, Size, 0>::binary(*this, o,
            [](scalar& a, scalar b) constexpr noexcept { a -= b; }
        );

        return *this;
    }

    constexpr VecN operator-(const VecN& o) const noexcept
    {
        VecN c = *this;
        return c -= o;
    }

    constexpr VecN& operator*=(scalar o) noexcept
    {
        detail::VecGetter<scalar, Size, 0>::unary(*this,
            [o](scalar& a) constexpr noexcept { a *= o; }
        );
        return *this;
    }

    constexpr VecN operator*(scalar o) const noexcept
    {
        VecN c = *this;
        return c *= o;
    }

    constexpr VecN& operator/=(scalar o) noexcept
    {
        detail::VecGetter<scalar, Size, 0>::unary(*this,
            [o](scalar& a) constexpr noexcept { a /= o; }
        );
        return *this;
    }

    constexpr VecN operator/(scalar o) const noexcept
    {
        VecN c = *this;
        return c /= o;
    }

    constexpr bool operator==(const VecN& o) const noexcept
    {
        return detail::VecGetter<scalar, Size, 0>::equal(*this, o);
    }


    constexpr bool operator!=(const VecN& o) const noexcept
    {
        return !(*this == o);
    }

    constexpr VecN lerp(const VecN& other, scalar factor) const noexcept
    {
        return *this * (1-factor) + other * factor;
    }

    constexpr ScalarT length_squared() const noexcept
    {
        return detail::VecGetter<scalar, Size, 0>::length(*this);
    }

    ScalarT length() const noexcept
    {
        return std::sqrt(length_squared());
    }

    VecN& normalize() noexcept
    {
        return *this /= length();
    }

    VecN normalized() const noexcept
    {
        return *this / length();
    }

private:
    scalar v_;

    template<class, int, int> friend struct detail::VecGetter;
};

template<class ScalarT> class VecN<ScalarT, 0>
{
public:
    constexpr VecN() noexcept {}
    template<class... T> constexpr VecN(const std::tuple<T...>&) noexcept {}
};

namespace detail {

/**
 * Just ensures all types are correct
 */
template<class Derived, class ScalarT, int Size>
class VecCRTP : public VecN<ScalarT, Size>
{
private:
    using Base = VecN<ScalarT, Size>;

public:
    using scalar = typename Base::scalar;
    using Base::Base;

    constexpr Derived operator-() const noexcept { return -b(); }
    constexpr Derived& operator+=(const Base& o) noexcept { return d(b() += o); }
    constexpr Derived& operator-=(const Base& o) noexcept { return d(b() -= o); }
    constexpr Derived& operator*=(scalar o) noexcept { return d(b() *= o); }
    constexpr Derived& operator/=(scalar o) noexcept { return d(b() /= o); }
    constexpr Derived operator+(const Base& o) const noexcept { return Derived(d()) += o; }
    constexpr Derived operator-(const Base& o) const noexcept { return Derived(d()) -= o; }
    constexpr Derived operator*(scalar o) const noexcept { return Derived(d()) *= o; }
    constexpr Derived operator/(scalar o) const noexcept { return Derived(d()) /= o; }
    constexpr Derived lerp(const Base& other, scalar factor) const noexcept { return d() * (1-factor) + other * factor; }
    Derived& normalize() noexcept { return d(b().normalize()); }
    Derived normalized() const noexcept { return d() / this->length(); }

protected:
    using Ctor = VecCRTP;

private:
    constexpr Base& b() noexcept { return *static_cast<Base*>(this); }
    constexpr const Base& b() const noexcept { return *static_cast<const Base*>(this); }
    constexpr Derived& d() noexcept { return *static_cast<Derived*>(this); }
    constexpr const Derived& d() const noexcept { return *static_cast<const Derived*>(this); }
    static constexpr Derived& d(Base& b) noexcept { return static_cast<Derived&>(b); }
    static constexpr const Derived& d(const Base& b) noexcept { return static_cast<const Derived&>(b); }
};

} // namespace detail


class Vec2 : public detail::VecCRTP<Vec2, double, 2>
{
    Q_GADGET
public:
    using Ctor::Ctor;

    static Vec2 from_polar(scalar length, scalar angle)
    {
        return Vec2{std::cos(angle) * length, std::sin(angle) * length};
    }

    constexpr scalar x() const noexcept { return get<0>(); }
    constexpr scalar& x() noexcept { return get<0>(); }
    constexpr void set_x(scalar v) noexcept { get<0>() = v; }

    constexpr scalar y() const noexcept { return get<1>(); }
    constexpr scalar& y() noexcept { return get<1>(); }
    constexpr void set_y(scalar v) noexcept { get<1>() = v; }
};

class Vec3 : public detail::VecCRTP<Vec3, double, 3>
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

class Vec4 : public detail::VecCRTP<Vec4, double, 4>
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


} // namespace math
