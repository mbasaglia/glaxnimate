#include "bezier_length.hpp"



glaxnimate::math::bezier::LengthData::LengthData(const Solver& segment, int steps)
{
    if ( steps == 0 )
        return;

    children_.reserve(steps);

    QPointF p = segment.points()[0];

    for ( int i = 1; i <= steps; i++ )
    {
        qreal t = qreal(i) / steps;
        QPointF q = segment.solve(t);
        auto l = math::length(p - q);
        length_ += l;
        children_.push_back({t, l, length_});
        p = q;
    }
}

glaxnimate::math::bezier::LengthData::LengthData(const Bezier& bez, int steps)
{
    children_.reserve(bez.size());
    int count = bez.segment_count();

    for ( int i = 0; i < count; i++ )
    {
        children_.emplace_back(bez.segment(i), steps);
        length_ += children_.back().length_;
        children_.back().cumulative_length_ = length_;
    }
}

glaxnimate::math::bezier::LengthData::LengthData(const MultiBezier& mbez, int steps)
{
    children_.reserve(mbez.size());

    for ( const auto& bez : mbez.beziers() )
    {
        children_.emplace_back(bez, steps);
        length_ += children_.back().length_;
        children_.back().cumulative_length_ = length_;
    }
}


glaxnimate::math::bezier::LengthData::SplitInfo glaxnimate::math::bezier::LengthData::at_ratio(qreal ratio) const
{
    return at_length(length_ * ratio);
}

glaxnimate::math::bezier::LengthData::SplitInfo glaxnimate::math::bezier::LengthData::at_length(qreal length) const
{
    if ( length <= 0 )
        return {0, 0., 0., &children_.front()};

    if ( length >= length_ )
        return {
            int(children_.size() - 1),
            1.,
            length - (children_.size() == 1 ? 0 : children_[children_.size() - 2].length_),
            &children_.back()
        };

    qreal prev_length = 0;

    for ( int i = 0; i < int(children_.size()); i++ )
    {
        const auto& child = children_[i];

        if ( child.cumulative_length_ > length )
        {
            qreal residual_length = length - prev_length;
            qreal ratio = qFuzzyIsNull(child.length_) ? 0 : residual_length / child.length_;
            if ( child.leaf_ )
                ratio = math::lerp(i == 0 ? 0 : children_[i - 1].t_, children_[i].t_, ratio);
            return {i, ratio, residual_length, &child};
        }

        prev_length = child.cumulative_length_;
    }

    return {int(children_.size() - 1), 1., length, &children_.back()};
}

qreal glaxnimate::math::bezier::LengthData::length() const noexcept
{
    return length_;
}

glaxnimate::math::bezier::LengthData::LengthData(qreal t, qreal length, qreal cumulative_length)
    : t_(t), length_(length), cumulative_length_(cumulative_length), leaf_(true)
{}

qreal glaxnimate::math::bezier::LengthData::from_ratio(qreal ratio) const
{
    if ( ratio <= 0 )
        return 0;

    if ( ratio >= 1 )
        return length_;

    for ( int i = 0; i < int(children_.size()); i++ )
    {
        if ( qFuzzyCompare(children_[i].t_, ratio) )
            return children_[i].cumulative_length_;

        if ( children_[i].t_ >= ratio )
        {
            if ( i == 0 )
            {
                qreal factor = ratio * children_[i].t_;
                return factor * children_[i].cumulative_length_;
            }

            qreal factor = (ratio - children_[i-1].t_) * (children_[i].t_ - children_[i-1].t_);
            return math::lerp(children_[i-1].cumulative_length_, children_[i].cumulative_length_, factor);
        }
    }

    return length_;
}

