#pragma once

#include <array>

#include "glaxnimate/math/vector.hpp"

namespace glaxnimate::math::bezier {

template<class Vec>
class CubicBezierSolver
{
public:
    using vector_type = Vec;
    using scalar = scalar_type<Vec>;

    constexpr CubicBezierSolver(Vec p0, Vec p1, Vec p2, Vec p3) noexcept
    : points_{p0, p1, p2, p3} {}

    constexpr CubicBezierSolver(const std::array<Vec, 4>& points) noexcept
    : points_(points) {}

    /**
     * \brief Finds the point along the bezier curve
     * \param factor between 0 and 1
     */
    constexpr Vec solve(scalar factor) const noexcept
    {
        Vec v;
        for ( int i = 0; i < detail::VecSize<Vec>::value; i++ )
            detail::get(v, i) = solve_component(factor, i);
        return v;
    }

    constexpr scalar solve_component(scalar factor, int component) const noexcept
    {
        return fast_cubic(
            factor,
            detail::get(points_[0], component),
            detail::get(points_[1], component),
            detail::get(points_[2], component),
            detail::get(points_[3], component)
        );
    }

    constexpr scalar derivative(scalar factor, int component) const noexcept
    {
        return (
            3 * a(detail::get(points_[0], component), detail::get(points_[1], component), detail::get(points_[2], component), detail::get(points_[3], component)) * factor +
            2 * b(detail::get(points_[0], component), detail::get(points_[1], component), detail::get(points_[2], component))
        ) * factor + c(detail::get(points_[0], component), detail::get(points_[1], component));
    }

    /**
     * \brief Finds the tangent of the point on the bezier
     * \param factor between 0 and 1
     * \return Angle in radians
     */
    scalar tangent_angle(scalar factor) const
    {
        return std::atan2(derivative(factor, 1), derivative(factor, 0));
    }

    constexpr const std::array<Vec, 4>& points() const noexcept
    {
        return points_;
    }

    constexpr std::array<Vec, 4>& points() noexcept
    {
        return points_;
    }

    /**
     * \brief Splits a bezier
     * \param factor value between 0 and 1 determining the split position
     * \return Two vectors for the two resulting cubic beziers
     */
    std::pair<std::array<Vec, 4>, std::array<Vec, 4>> split(scalar factor) const
    {
        // linear
        if ( points_[0] == points_[1] && points_[2] == points_[3] )
        {
            Vec mid = lerp(points_[0], points_[3], factor);
            return {
                {points_[0], points_[0], mid, mid},
                {mid, mid, points_[3], points_[3]},
            };
        }

        Vec p01 = lerp(points_[0], points_[1], factor);
        Vec p12 = lerp(points_[1], points_[2], factor);
        Vec p23 = lerp(points_[2], points_[3], factor);

        Vec p012 = lerp(p01, p12, factor);
        Vec p123 = lerp(p12, p23, factor);

        Vec p0123 = lerp(p012, p123, factor);

        return {
            {points_[0], p01, p012, p0123},
            {p0123, p123, p23, points_[3]}
        };
    }

    std::pair<Vec, Vec> bounds()
    {
        std::vector<scalar> solutions;
        for ( int i = 0; i < detail::VecSize<Vec>::value; i++ )
        {
            scalar p0 = detail::get(points_[0], i);
            scalar p1 = detail::get(points_[1], i);
            scalar p2 = detail::get(points_[2], i);
            scalar p3 = detail::get(points_[3], i);
            scalar c_a = 3 * p3 - 9 * p2 + 9 * p1 - 3 * p0;
            scalar c_b = 6 * p2 - 12 * p1 + 6 * p0;
            scalar c_c = 3 * p1 - 3 * p0;
            bounds_solve(c_a, c_b, c_c, solutions);
        }

        std::vector<Vec> boundary_points;
        boundary_points.push_back(points_[0]); //Add Begin and end point not the control points!
        boundary_points.push_back(points_[3]);

        for ( scalar e : solutions )
            boundary_points.push_back(solve(e));

        Vec min;
        Vec max;
        for ( int i = 0; i < detail::VecSize<Vec>::value; i++ )
        {
            scalar cmin = std::numeric_limits<scalar>::max();
            scalar cmax = std::numeric_limits<scalar>::lowest();
            for ( const Vec& p : boundary_points )
            {
               if ( detail::get(p, i) < cmin )
                   cmin = detail::get(p, i);
               if ( detail::get(p, i) > cmax )
                   cmax = detail::get(p, i);
            }
            detail::get(max, i) = cmax;
            detail::get(min, i) = cmin;
        }

        return {min, max};
    }

private:
    // You get these terms by expanding the Bezier definition and re-arranging them as a polynomial in t
    // B(t) = a t^3 + b t^2 + c t + d
    static constexpr scalar a(const scalar& k0, const scalar& k1, const scalar& k2, const scalar& k3) noexcept
    {
        return -k0 + k1*3 + k2 * -3 + k3;
    }
    static constexpr scalar b(const scalar& k0, const scalar& k1, const scalar& k2) noexcept
    {
        return k0 * 3 + k1 * -6 + k2 * 3;
    }
    static constexpr scalar c(const scalar& k0, const scalar& k1) noexcept
    {
        return k0 * -3 + k1 * 3;
    }
    static constexpr scalar d(const scalar& k0) noexcept
    {
        return k0;
    }
    static constexpr scalar fast_cubic(scalar t, const scalar& k0, const scalar& k1, const scalar& k2, const scalar& k3) noexcept
    {
        return ((a(k0, k1, k2, k3) * t + b(k0, k1, k2) ) * t +  c(k0, k1) ) * t + d(k0);
    }

    static void bounds_solve(scalar a, scalar b, scalar c, std::vector<scalar>& solutions)
    {
        scalar d = b * b - 4. * a * c;

        // no real solution
        if ( d < 0 )
            return;

        if ( a == 0 )
        {
            // linear case
            add_bounds_solution(-c / b, solutions);
        }
        else
        {
            add_bounds_solution((-b + std::sqrt(d)) / (2 * a), solutions);

            if ( d != 0 )
                add_bounds_solution((-b - std::sqrt(d)) / (2 * a), solutions);
        }
    }

    static void add_bounds_solution(scalar solution, std::vector<scalar>& solutions)
    {
        if ( solution >= 0 && solution <= 1 )
            solutions.push_back(solution);
    }

    std::array<Vec, 4> points_;
};

} // namespace glaxnimate::math::bezier
