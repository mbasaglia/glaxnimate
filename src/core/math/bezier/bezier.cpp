#include "bezier.hpp"

using namespace glaxnimate;

QRectF math::bezier::Bezier::bounding_box() const
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

    if ( closed_ )
    {
        pair = solver_for_point(size()-1).bounds();
        box |= QRectF(pair.first, pair.second);
    }

    return box;
}

void math::bezier::Bezier::split_segment(int index, qreal factor)
{
    if ( points_.empty() )
        return;

    if ( index < 0 )
    {
        points_.insert(points_.begin(), points_[0]);
        return;
    }
    else if ( index >= size() )
    {
        points_.insert(points_.end(), points_.back());
        return;
    }

    auto split_points = solver_for_point(index).split(factor);
    points_[index].tan_out = split_points.first[1];
    points_[(index+1) % size()].tan_in = split_points.second[2];

    auto type = Smooth;
    if ( factor <= 0 )
        type = points_[index].type;
    else if ( factor >= 1 )
        type = points_[(index+1) % size()].type;

    points_.insert(points_.begin() + index + 1, Point(
        split_points.first[3],
        split_points.first[2],
        split_points.second[1],
        type
    ));
}

math::bezier::Point math::bezier::Bezier::split_segment_point(int index, qreal factor) const
{
    if ( index < 0 )
        return points_[0];
    else if ( index >= size() )
        return points_.back();

    if ( factor <= 0 )
        return points_[index];
    else if ( factor >= 1 )
        return points_[(index+1) % size()];

    auto split_points = solver_for_point(index).split(factor);
    return Point(
        split_points.first[3],
        split_points.first[2],
        split_points.second[1],
        Smooth
    );
}

void math::bezier::Bezier::add_to_painter_path(QPainterPath& out) const
{
    if ( size() < 2 )
        return;

    out.moveTo(points_[0].pos);
    for ( int i = 1; i < size(); i++ )
    {
        out.cubicTo(points_[i-1].tan_out, points_[i].tan_in, points_[i].pos);
    }

    if ( closed_ )
    {
        out.cubicTo(points_.back().tan_out, points_[0].tan_in, points_[0].pos);
        out.closeSubpath();
    }
}

math::bezier::Bezier math::bezier::Bezier::lerp(const math::bezier::Bezier& other, qreal factor) const
{
    if ( other.closed_ != closed_ || other.size() != size() )
        return *this;

    math::bezier::Bezier lerped;
    lerped.closed_ = closed_;
    lerped.points_.reserve(size());
    for ( int i = 0; i < size(); i++ )
        lerped.points_.push_back(Point::from_relative(
            math::lerp(points_[i].pos, other.points_[i].pos, factor),
            math::lerp(
                points_[i].tan_in - points_[i].pos,
                other.points_[i].tan_in - other.points_[i].pos,
                factor
            ),
            math::lerp(
                points_[i].tan_out - points_[i].pos,
                other.points_[i].tan_out - other.points_[i].pos,
                factor
            )
        ));
    return lerped;
}


void math::bezier::Bezier::reverse()
{
    std::reverse(points_.begin(), points_.end());

    if ( closed_ && points_.size() > 1 )
    {
        auto back = points_.back();
        points_.pop_back();
        points_.insert(points_.begin(), back);
    }

    for ( auto& p : points_ )
        std::swap(p.tan_in, p.tan_out);
}

math::bezier::BezierSegment math::bezier::Bezier::segment(int index) const
{
    return {
        points_[index].pos,
        points_[index].tan_out,
        points_[(index+1) % points_.size()].tan_in,
        points_[(index+1) % points_.size()].pos
    };
}

math::bezier::BezierSegment math::bezier::Bezier::inverted_segment(int index) const
{
    return {
        points_[(index+1) % points_.size()].pos,
        points_[(index+1) % points_.size()].tan_in,
        points_[index].tan_out,
        points_[index].pos
    };
}

void math::bezier::Bezier::set_segment(int index, const math::bezier::BezierSegment& s)
{
    points_[index].pos = s[0];
    points_[index].drag_tan_out(s[1]);
    points_[(index+1) % points_.size()].pos = s[3];
    points_[(index+1) % points_.size()].drag_tan_in(s[2]);
}

math::bezier::Bezier math::bezier::Bezier::transformed(const QTransform& t) const
{
    auto copy = *this;
    copy.transform(t);
    return copy;
}

void math::bezier::Bezier::transform(const QTransform& t)
{
    for ( auto& p : points_ )
        p.transform(t);
}

int glaxnimate::math::bezier::Bezier::segment_count() const
{
    return closed_ || points_.empty() ? points_.size() : points_.size() - 1;
}

QRectF math::bezier::MultiBezier::bounding_box() const
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

void math::bezier::MultiBezier::append(const QPainterPath& path)
{
    std::array<QPointF, 3> data;
    int data_i = 0;
    for ( int i = 0; i < path.elementCount(); i++ )
    {
        auto element = path.elementAt(i);
        switch ( element.type )
        {
            case QPainterPath::MoveToElement:
                if ( !beziers_.empty() && beziers_.back()[0].pos == beziers_.back().back().pos )
                    close();
                move_to(element);
                break;
            case QPainterPath::LineToElement:
                line_to(element);
                break;
            case QPainterPath::CurveToElement:
                data_i = 0;
                data[0] = element;
                break;
            case QPainterPath::CurveToDataElement:
                ++data_i;
                data[data_i] = element;
                if ( data_i == 2 )
                {
                    cubic_to(data[0], data[1], data[2]);
                    data_i = -1;
                }
                break;
        }
    }
}

void math::bezier::MultiBezier::transform(const QTransform& t)
{
    for ( auto& bez : beziers_ )
        bez.transform(t);
}

math::bezier::MultiBezier math::bezier::MultiBezier::from_painter_path(const QPainterPath& path)
{
    math::bezier::MultiBezier bez;
    bez.append(path);
    return bez;
}
