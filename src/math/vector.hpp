#pragma once

#include <tuple>
#include <utility>
#include <cmath>

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

} // namespace math
