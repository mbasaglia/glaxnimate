#include "bezier.hpp"


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
        Smooth
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
                points_[i].tan_out - points_[i].pos,
                other.points_[i].tan_out - other.points_[i].pos,
                factor
            ),
            math::lerp(
                points_[i].tan_in - points_[i].pos,
                other.points_[i].tan_in - other.points_[i].pos,
                factor
            )
        ));
    return lerped;
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
