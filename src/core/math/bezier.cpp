#include "bezier.hpp"




QRectF math::Bezier::bounding_box() const
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

void math::Bezier::split_segment(int index, qreal factor)
{
    if ( index <= 0 )
    {
        points_.insert(points_.begin(), points_[0]);
        return;
    }
    else if ( index >= size() )
    {
        points_.insert(points_.end(), points_.back());
        return;
    }

    auto split_points = solver_for_point(index-1).split(factor);
    points_[index-1].tan_out = split_points.first[1];
    points_[index].tan_in = split_points.second[2];
    points_.insert(points_.begin() + index, BezierPoint(
        split_points.first[3],
        split_points.first[2],
        split_points.second[1]
    ));
}


void math::Bezier::add_to_painter_path(QPainterPath& out) const
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

math::Bezier math::Bezier::lerp(const math::Bezier& other, qreal factor) const
{
    if ( other.closed_ != closed_ || other.size() != size() )
        return *this;

    math::Bezier lerped;
    lerped.closed_ = closed_;
    lerped.points_.reserve(size());
    for ( int i = 0; i < size(); i++ )
        lerped.points_.push_back(BezierPoint::from_relative(
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


QRectF math::MultiBezier::bounding_box() const
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
