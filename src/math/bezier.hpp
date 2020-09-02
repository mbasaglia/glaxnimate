#pragma once

#include <QPointF>
#include "math/bezier_solver.hpp"
#include "math/bezier_point.hpp"

namespace math {

class Bezier
{
public:
    Bezier() = default;
    explicit Bezier(const BezierPoint& initial_point)
        : points_(1, initial_point)
    {}
    explicit Bezier(const QPointF& initial_point)
        : points_(1, initial_point)
    {}

    const std::vector<BezierPoint>& points() const { return points_; }
    std::vector<BezierPoint>& points() { return points_; }

    int size() const { return points_.size(); }
    bool empty() const { return points_.empty(); }
    auto begin() { return points_.begin(); }
    auto begin() const { return points_.begin(); }
    auto cbegin() const { return points_.begin(); }
    auto end() { return points_.end(); }
    auto end() const { return points_.end(); }
    auto cend() const { return points_.end(); }
    void push_back(const BezierPoint& p) { points_.push_back(p); }

    const BezierPoint& operator[](int index) const { return points_[index]; }
    BezierPoint& operator[](int index) { return points_[index]; }

    bool closed() const { return closed_; }
    void set_closed(bool closed) { closed_ = closed; }

    /**
     * \brief Inserts a point at the given index
     * \param index Index to insert the point at
     * \param p     Point to add
     * \returns \c this, for easy chaining
     */
    Bezier& insert_point(int index, const BezierPoint& p)
    {
        points_.insert(points_.begin() + qBound(0, index, size()), p);
        return *this;
    }

    /**
     * \brief Appends a point to the curve (relative tangents)
     * \see insert_point()
     */
    Bezier& add_point(const QPointF& p, const QPointF& in_t = {0, 0}, const QPointF& out_t = {0, 0})
    {
        points_.push_back(BezierPoint::from_relative(p, in_t, out_t));
        return *this;
    }

    /**
     * \brief Appends a point with symmetrical (relative) tangents
     * \see insert_point()
     */
    Bezier& add_smooth_point(const QPointF& p, const QPointF& in_t)
    {
        return add_point(p, in_t, -in_t);
    }

    /**
     * \brief Closes the bezier curve
     * \returns \c this, for easy chaining
     */
    Bezier& close()
    {
        closed_ = true;
        return *this;
    }

    /**
     * \brief Line from the last point to \p p
     * \returns \c this, for easy chaining
     */
    Bezier& line_to(const QPointF& p)
    {
        if ( !empty() )
            points_.back().tan_out = points_.back().pos;
        points_.push_back(p);
        return *this;
    }

    /**
     * \brief Quadratic bezier from the last point to \p dest
     * \param handle Quadratic bezier handle
     * \param dest   Destination point
     * \returns \c this, for easy chaining
     */
    Bezier& quadratic_to(const QPointF& handle, const QPointF& dest)
    {
        if ( !empty() )
            points_.back().tan_out = points_.back().pos + 2.0/3.0 * (handle - points_.back().pos);

        push_back(dest);
        points_.back().tan_in = points_.back().pos + 2.0/3.0 * (handle - points_.back().pos);

        return *this;
    }

    /**
     * \brief Cubic bezier from the last point to \p dest
     * \param handle1   First cubic bezier handle
     * \param handle2   Second cubic bezier handle
     * \param dest      Destination point
     * \returns \c this, for easy chaining
     */
    Bezier& cubic_to(const QPointF& handle1, const QPointF& handle2, const QPointF& dest)
    {
        if ( !empty() )
            points_.back().tan_out = handle1;

        push_back(dest);
        points_.back().tan_in = handle2;

        return *this;
    }

    /**
     * \brief Reverses the orders of the points
     */
    void reverse()
    {
        std::reverse(points_.begin(), points_.end());
        for ( auto& p : points_ )
            std::swap(p.tan_in, p.tan_out);
    }

    QRectF bounding_box() const
    {
        if ( size() < 2 )
            return {};

        auto pair = solver_for_point(0).bounds();
        QRectF box(pair.first, pair.second);
        for ( int i = 1; i < size() - 1; i++ )
        {
            pair = solver_for_point(i).bounds();
            box |= QRectF(pair.first, pair.second);
        }
        return box;
    }

    Bezier rounded(qreal radius)
    {
        Bezier cloned;
        cloned.closed_ = closed_;

        for ( int i = 0; i < size(); i++ )
        {
            if ( !closed_ && (i == 0 || i == size() - 1) )
            {
                cloned.push_back(points_[i]);
            }
            else
            {
                QPointF vert1, out_t, vert2, in_t;
                std::tie(vert1, out_t) = round_vert_tan(i - 1, radius, points_[i].pos);
                cloned.push_back(BezierPoint(vert1, vert1, vert1 + out_t));
                std::tie(vert2, in_t) = round_vert_tan((i+1) % size(), radius, points_[i].pos);
                cloned.push_back(BezierPoint(vert2, vert2+in_t, vert2));
            }
        }

        return cloned;
    }

private:
    /**
     * \brief Solver for the point \p p to the point \p p + 1
     */
    math::CubicBezierSolver<QPointF> solver_for_point(int p) const
    {
        return {
            points_[p].pos,
            points_[p].tan_out,
            points_[p+1].tan_in,
            points_[p+1].pos
        };
    }

    std::pair<QPointF, QPointF> round_vert_tan(
        int closest_index, qreal radius, const QPointF& current
    )
    {
        const qreal round_corner = 0.5519;
        QPointF closer_v = points_[closest_index].pos;
        qreal distance = length(current - closer_v);
        qreal new_pos_perc = distance != 0 ? qMin(distance/2, radius) / distance : 0;
        QPointF vert = current + (closer_v - current) * new_pos_perc;
        QPointF tan = - (vert - current) * round_corner;
        return {vert, tan};
    }

    std::vector<BezierPoint> points_;
    bool closed_ = false;
};


class MultiBezier
{
public:
    const std::vector<Bezier>& beziers() const { return beziers_; }
    std::vector<Bezier>& beziers() { return beziers_; }

    MultiBezier& move_to(const QPointF& p)
    {
        beziers_.push_back(Bezier(p));
        at_end = false;
        return *this;
    }

    MultiBezier& line_to(const QPointF& p)
    {
        handle_end();
        beziers_.back().line_to(p);
        return *this;
    }

    MultiBezier& quadratic_to(const QPointF& handle, const QPointF& dest)
    {
        handle_end();
        beziers_.back().quadratic_to(handle, dest);
        return *this;
    }

    MultiBezier& cubic_to(const QPointF& handle1, const QPointF& handle2, const QPointF& dest)
    {
        handle_end();
        beziers_.back().cubic_to(handle1, handle2, dest);
        return *this;
    }

    MultiBezier& close()
    {
        if ( !beziers_.empty() )
            beziers_.back().close();
        at_end = true;
        return *this;
    }

    QRectF bounding_box() const
    {
        if ( beziers_.empty() )
            return {};

        QRectF box;
        for ( const Bezier& bez : beziers_ )
        {
            QRectF bb = bez.bounding_box();
            if ( box.isNull() )
                box = bb;
            else if ( !bb.isNull() )
                box |= bb;
        }
        return box;
    }

    QPainterPath painter_path() const
    {
        QPainterPath p;
        for ( const Bezier& bez : beziers_ )
        {
            if ( bez.size() < 2 )
                continue;

            p.moveTo(bez[0].pos);
            for ( int i = 1; i < bez.size(); i++ )
                p.cubicTo(bez[i-1].tan_out, bez[i].tan_in, bez[i].pos);

            if ( bez.closed() )
            {
                p.cubicTo(bez.points().back().tan_out, bez[0].tan_in, bez[0].pos);
                p.closeSubpath();
            }
        }
        return p;
    }

private:
    void handle_end()
    {
        if ( at_end )
        {
            beziers_.push_back(Bezier());
            if ( beziers_.size() > 1 )
                beziers_.back().add_point(beziers_[beziers_.size()-2].points().back().pos);
            at_end = false;
        }
    }

    std::vector<Bezier> beziers_;
    bool at_end = true;
};

} // namespace math
