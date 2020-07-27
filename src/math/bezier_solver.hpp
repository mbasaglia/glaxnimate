#pragma once

#include "math/vector.hpp"

namespace math {

template<class T>
auto binom(T n, T k)
{
    return 1 / ( (n+1) * std::beta(n-k+1, k+1) );
}

template<class Vec>
class BezierSolver
{
public:
    using scalar = typename Vec::scalar;

    /**
     * \param points Bezier points (absolute values)
     */
    BezierSolver(std::vector<Vec> points) : points_(std::move(points)) {}

    BezierSolver(std::initializer_list<Vec> points) : points_(std::move(points)) {}

    /**
     * \brief Reduces the order in case of duplicate points
     */
    void optimize()
    {
        if ( points_.size() <= 2 )
            return;

        std::vector<Vec> new_points;
        new_points.reserve(points_.size());
        new_points.push_back(points_[0]);
        for ( std::size_t i = 1; i < points_.size(); i++ )
            if ( points_[i] != new_points.back() )
                new_points.push_back(points_[i]);

        if ( new_points.size() == 1 )
            new_points.push_back(points_.back());

        points_ = std::move(new_points);
    }

    /**
     * \brief Bezier order (eg: 1 for linear etc)
     */
    int order() const
    {
        return points_.size() - 1;
    }

    /**
     * \brief Finds the point along the bezier curve
     * \param factor between 0 and 1
     */
    Vec solve(scalar factor) const
    {
        if ( order() <=  0 )
            return points_[0];

        Vec p;
        for ( int i = 0; i < int(points_.size()); i++ )
            p += points_[i] * coefficient(i, order(), factor);
        return p;
    }

    /**
     * \brief Finds the tangent of the point on the bezier
     * \param factor between 0 and 1
     * \return Angle in radians
     */
    scalar tangent_angle(scalar factor) const
    {
        if ( order() <= 0 )
            return 0;

        Vec p;
        for ( int i = 0; i < int(points_.size()) - 1; i++ )
            p += (points_[i+1] - points_[i]) * order() * coefficient(i, order() - 1, factor);
        return std::atan2(p.y(), p.x());
    }

    /**
     * \brief Performs a bezier step, reducing the order of the points
     * \returns Bezier points of a smaller order
     */
    std::vector<Vec> solve_step(scalar factor) const
    {
        if ( order() < 1 )
            return points_;

        std::vector<Vec> next;
        Vec p1 = points_[0];
        for ( auto it = points_.begin() + 1; it != points_.end(); ++it )
        {
            next.push_back(p1.lerp(*it, factor));
            p1 = *it;
        }
        return next;
    }

    /**
     * \brief Splits a bezier
     * \param factor value between 0 and 1 determining the split position
     * \return Two vectors for the two resulting cubic beziers
     */
    std::pair<std::vector<Vec>, std::vector<Vec>> split_cubic(scalar factor) const
    {
        if ( order() == 1 )
        {
            auto k = points_[0].lerp(points_[1], factor);
            return {
                {points_[0], points_[0], k, k},
                {k, k, points_[1], points_[1]},
            };
        }

        BezierSolver quad_solv(order() == 2 ? points_ : solve_step(factor));
        auto lin = quad_solv.solve_step(factor);
        auto k = lin[0].lerp(lin[1], factor);
        return {
            {points_.front(), quad_solv.points_.front(), lin.front(), k},
            {k, lin.back(), quad_solv.points_.back(), points_.back()}
        };

    }

    const std::vector<Vec>& points() const
    {
        return points;
    }

private:
    scalar coefficient(int point_index, int order, scalar factor) const
    {
        return binom(order, point_index)
            * std::pow(factor, point_index)
            * std::pow(1-factor, order - point_index)
        ;
    }

    std::vector<Vec> points_;
};

} // namespace math
