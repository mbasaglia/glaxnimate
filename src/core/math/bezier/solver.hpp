#pragma once

#include <array>

#include "math/vector.hpp"
#include "math/polynomial.hpp"

#include <QPointF>

namespace glaxnimate::math::bezier {

template<class Vec>
class CubicBezierSolver
{
public:
    using vector_type = Vec;
    using scalar = scalar_type<Vec>;
    using argument_type = scalar;
    using bounding_box_type = std::pair<Vec, Vec>;


    constexpr CubicBezierSolver(const std::array<Vec, 4>& points) noexcept
    : points_(points)
    {
        rebuild_coeff();
    }

    constexpr CubicBezierSolver(Vec p0, Vec p1, Vec p2, Vec p3) noexcept
    : CubicBezierSolver{{p0, p1, p2, p3}} {}

    /**
     * \brief Finds the point along the bezier curve
     * \param t between 0 and 1
     */
    constexpr Vec solve(argument_type t) const noexcept
    {
        return ((a_ * t + b_ ) * t +  c_ ) * t + d_;
    }

    constexpr scalar solve_component(argument_type t, int component) const noexcept
    {
        scalar a = detail::get(a_, component);
        scalar b = detail::get(b_, component);
        scalar c = detail::get(c_, component);
        scalar d = detail::get(d_, component);
        return ((a * t + b ) * t +  c ) * t + d;
    }

    constexpr scalar derivative(argument_type factor, int component) const noexcept
    {
        scalar a = detail::get(a_, component);
        scalar b = detail::get(b_, component);
        scalar c = detail::get(c_, component);
        return (3 * a * factor + 2 * b) * factor + c;
    }

    /**
     * \brief Finds the tangent of the point on the bezier
     * \param factor between 0 and 1
     * \return Angle in radians
     */
    scalar tangent_angle(argument_type factor) const
    {
        return std::atan2(derivative(factor, 1), derivative(factor, 0));
    }

    constexpr const std::array<Vec, 4>& points() const noexcept
    {
        return points_;
    }

//     constexpr std::array<Vec, 4>& points() noexcept
//     {
//         return points_;
//     }

    template<int i>
    constexpr void set(const Vec& v) noexcept
    {
        points_[i] = v;
        rebuild_coeff();
    }

    /**
     * \brief Splits a bezier
     * \param factor value between 0 and 1 determining the split position
     * \return Two vectors for the two resulting cubic beziers
     */
    std::pair<std::array<Vec, 4>, std::array<Vec, 4>> split(argument_type factor) const
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

    bounding_box_type bounds() const
    {
        std::vector<scalar> solutions;
        for ( int i = 0; i < detail::VecSize<Vec>::value; i++ )
        {
            bounds_solve(3 * detail::get(a_, i), 2 * detail::get(b_, i), detail::get(c_, i), solutions);
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

    /**
     * \brief Return inflection points for a 2D bezier
     */
    std::vector<argument_type> inflection_points() const
    {
        auto denom = detail::get(a_, 1) * detail::get(b_, 0) - detail::get(a_, 0) * detail::get(b_, 1);
        if ( qFuzzyIsNull(denom) )
            return {};

        auto t_cusp = -0.5 * (detail::get(a_, 1) * detail::get(c_, 0) - detail::get(a_, 0) * detail::get(c_, 1)) / denom;

        auto square = t_cusp * t_cusp - 1./3. * (detail::get(b_, 1) * detail::get(c_, 0) - detail::get(b_, 0) * detail::get(c_, 1)) / denom;

        if ( square < 0 )
            return {};

        auto root = std::sqrt(square);
        if ( qFuzzyIsNull(root) )
        {
            if ( is_valid_inflection(t_cusp) )
                return {t_cusp};
            return {};
        }

        std::vector<argument_type> roots;
        roots.reserve(2);

        if ( is_valid_inflection(t_cusp - root) )
            roots.push_back(t_cusp - root);


        if ( is_valid_inflection(t_cusp + root) )
            roots.push_back(t_cusp + root);

        return roots;
    }


    std::vector<std::pair<argument_type, argument_type>> intersections(
        const CubicBezierSolver& other, scalar tolerance = 2, int max_recursion = 10
    ) const
    {
        std::vector<std::pair<argument_type, argument_type>> intersections;
        intersects_impl(IntersectData(*this), IntersectData(other), tolerance, intersections, 0, max_recursion);
        return intersections;
    }

    /**
     * \brief Returns the t corresponding to the given value for a component.
     * \returns `t` or -1 in case of no root has been found
     */
    scalar t_at_value(scalar value, int comp = 0) const
    {
        auto roots = math::cubic_roots(
            detail::get(a_, comp),
            detail::get(b_, comp),
            detail::get(c_, comp),
            detail::get(d_, comp) - value
        );

        for ( auto root : roots )
        {
            if ( root >= 0 && root <= 1 )
                return root;
            if ( qFuzzyIsNull(root) )
                return 0;
        }

        return -1;
    }

private:
    static constexpr bool is_valid_inflection(scalar root) noexcept
    {
        return root > 0 && root < 1;
    }

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

    struct IntersectData
    {
        IntersectData(
            const CubicBezierSolver& solver,
            const bounding_box_type& box,
            argument_type t1,
            argument_type t2
        ) : bez(solver),
            width(box.second.x() - box.first.x()),
            height(box.second.y() - box.first.y()),
            center((box.first + box.second) / 2),
            t1(t1),
            t2(t2),
            t((t1 + t2) / 2)
        {}

        IntersectData(const CubicBezierSolver& solver)
            : IntersectData(solver, solver.bounds(), 0, 1)
        {}

        std::pair<IntersectData, IntersectData> split() const
        {
            auto split = bez.split(0.5);
            return {
                IntersectData(split.first),
                IntersectData(split.second)
            };
        }

        bool intersects(const IntersectData& other) const
        {
            return std::abs(center.x() - other.center.x()) * 2 > width + other.width &&
                   std::abs(center.y() - other.center.y()) * 2 > height + other.height;
        }

        CubicBezierSolver bez;
        scalar width;
        scalar height;
        Vec center;
        argument_type t1, t2, t;
    };

    static void intersects_impl(
        const IntersectData& d1,
        const IntersectData& d2,
        scalar tolerance,
        std::vector<std::pair<argument_type, argument_type>>& intersections,
        int depth,
        int max_recursion
    )
    {
        if ( !d1.intersects(d2) )
            return;

        if ( depth >= max_recursion || (d1.width <= tolerance && d1.height <= tolerance && d2.width <= tolerance && d2.height <= tolerance) )
        {
            intersections.emplace_back(d1.t, d2.t);
            return;
        }

        auto d1s = d1.split();
        auto d2s = d2.split();

        intersects_impl(d1s.first, d2s.first, tolerance, intersections, depth + 1, max_recursion);
        intersects_impl(d1s.first, d2s.second, tolerance, intersections, depth + 1, max_recursion);
        intersects_impl(d1s.second, d2s.first, tolerance, intersections, depth + 1, max_recursion);
        intersects_impl(d1s.second, d2s.second, tolerance, intersections, depth + 1, max_recursion);
    }

    void rebuild_coeff()
    {
        for ( int component = 0; component < detail::VecSize<Vec>::value; component++ )
        {
            detail::get(a_, component) = a(
                detail::get(points_[0], component),
                detail::get(points_[1], component),
                detail::get(points_[2], component),
                detail::get(points_[3], component)
            );
            detail::get(b_, component) = b(
                detail::get(points_[0], component),
                detail::get(points_[1], component),
                detail::get(points_[2], component)
            );
            detail::get(c_, component) = c(
                detail::get(points_[0], component),
                detail::get(points_[1], component)
            );
            detail::get(d_, component) = d(
                detail::get(points_[0], component)
            );
        }
    }

    std::array<Vec, 4> points_;
    // Polynomial coefficients (a t^3 + b t^2 + c t + d = 0)
    Vec a_, b_, c_, d_;
};

} // namespace glaxnimate::math::bezier
