#pragma once

#include <QPointF>
#include <QPainterPath>
#include "math/bezier/solver.hpp"
#include "math/bezier/point.hpp"
#include "math/bezier/segment.hpp"

namespace glaxnimate::math::bezier {

class LengthData
{
public:
    struct SplitInfo
    {
        int index = 0;
        qreal ratio = 0;
        const LengthData* child = nullptr;

        SplitInfo child_split() const;
    };

    LengthData() = default;
    explicit LengthData(qreal length) : length_(length) {}
    explicit LengthData(const math::bezier::CubicBezierSolver<QPointF>& segment, int steps);

    template<class... Args>
    void add_child(Args&&... args)
    {
        children_.emplace_back(std::forward<Args>(args)...);
        length_ += children_.back().length_;
    }

    const std::vector<LengthData>& children() const
    {
        return children_;
    }

    qreal length() const
    {
        return length_;
    }

    void reserve(int size)
    {
        children_.reserve(size);
    }

    SplitInfo at_ratio(qreal ratio) const;
    SplitInfo at_length(qreal length) const;

private:
    qreal length_ = 0;
    std::vector<LengthData> children_;
};

class Bezier
{
public:
    using value_type = Point;

    Bezier() = default;
    explicit Bezier(const Point& initial_point)
        : points_(1, initial_point)
    {}
    explicit Bezier(const QPointF& initial_point)
        : points_(1, initial_point)
    {}

    const std::vector<Point>& points() const { return points_; }
    std::vector<Point>& points() { return points_; }

    int size() const { return points_.size(); }
    int closed_size() const { return points_.size() + (closed_ ? 1 : 0); }
    bool empty() const { return points_.empty(); }
    auto begin() { return points_.begin(); }
    auto begin() const { return points_.begin(); }
    auto cbegin() const { return points_.begin(); }
    auto end() { return points_.end(); }
    auto end() const { return points_.end(); }
    auto cend() const { return points_.end(); }
    void push_back(const Point& p) { points_.push_back(p); }
    void clear() { points_.clear(); closed_ = false; }
    const Point& back() const { return points_.back(); }
    Point& back() { return points_.back(); }

    const Point& operator[](int index) const { return points_[index % points_.size()]; }
    Point& operator[](int index) { return points_[index % points_.size()]; }

    bool closed() const { return closed_; }
    void set_closed(bool closed) { closed_ = closed; }

    /**
     * \brief Inserts a point at the given index
     * \param index Index to insert the point at
     * \param p     Point to add
     * \returns \c this, for easy chaining
     */
    Bezier& insert_point(int index, const Point& p)
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
        points_.push_back(Point::from_relative(p, in_t, out_t));
        return *this;
    }

    /**
     * \brief Appends a point with symmetrical (relative) tangents
     * \see insert_point()
     */
    Bezier& add_smooth_point(const QPointF& p, const QPointF& in_t)
    {
        points_.push_back(Point::from_relative(p, in_t, -in_t, Smooth));
        return *this;
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
    void reverse();

    QRectF bounding_box() const;

    /**
     * \brief Split a segmet
     * \param index index of the point at the beginning of the segment to split
     * \param factor Value between [0,1] to determine the split point
     * \post size() increased by one and points[index+1] is the new point
     */
    void split_segment(int index, qreal factor);

    /**
     * \brief The point you'd get by calling split_segment(index, factor)
     */
    Point split_segment_point(int index, qreal factor) const;

    void remove_point(int index)
    {
        if ( index >= 0 && index < size() )
            points_.erase(points_.begin() + index);
    }

    void add_to_painter_path(QPainterPath& out) const;

    math::bezier::Bezier lerp(const math::bezier::Bezier& other, qreal factor) const;

    void set_point(int index, const math::bezier::Point& p)
    {
        if ( index >= 0 && index < size() )
            points_[index] = p;
    }

    BezierSegment segment(int index) const;
    void set_segment(int index, const BezierSegment& s);
    BezierSegment inverted_segment(int index) const;

    int segment_count() const;

    Bezier transformed(const QTransform& t) const;
    void transform(const QTransform& t);

    /**
     * \brief Pre-computation for length-related operations
     */
    LengthData length_data(int steps) const;

private:
    /**
     * \brief Solver for the point \p p to the point \p p + 1
     */
    math::bezier::CubicBezierSolver<QPointF> solver_for_point(int p) const
    {
        return segment(p);
    }

    std::vector<Point> points_;
    bool closed_ = false;
};


class MultiBezier
{
public:
    const std::vector<Bezier>& beziers() const { return beziers_; }
    std::vector<Bezier>& beziers() { return beziers_; }

    Bezier& back() { return beziers_.back(); }
    const Bezier& back() const { return beziers_.back(); }

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

    QRectF bounding_box() const;

    QPainterPath painter_path() const
    {
        QPainterPath p;
        for ( const Bezier& bez : beziers_ )
            bez.add_to_painter_path(p);
        return p;
    }

    void append(const MultiBezier& other)
    {
        beziers_.insert(beziers_.end(), other.beziers_.begin(), other.beziers_.end());
    }

    void append(const QPainterPath& path);

    void transform(const QTransform& t);

    static MultiBezier from_painter_path(const QPainterPath& path);

    LengthData length_data(int steps) const;

    int size() const { return beziers_.size(); }
    bool empty() const { return beziers_.empty(); }

    const Bezier& operator[](int index) const
    {
        return beziers_[index];
    }

    Bezier& operator[](int index)
    {
        return beziers_[index];
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

} // namespace glaxnimate::math

Q_DECLARE_METATYPE(glaxnimate::math::bezier::Bezier)
