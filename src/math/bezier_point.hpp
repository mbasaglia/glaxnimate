#pragma once

#include <QPointF>

namespace math {

struct BezierPoint
{
    QPointF pos;
    QPointF tan_in;
    QPointF tan_out;

    BezierPoint(
        const QPointF& pos,
        const QPointF& tan_in,
        const QPointF& tan_out
    ) : pos(pos), tan_in(tan_in), tan_out(tan_out)
    {}

    BezierPoint(const QPointF& pos)
        : BezierPoint(pos, pos, pos)
    {}

    static BezierPoint from_relative(
        const QPointF& pos,
        const QPointF& tan_in_rel = {0, 0},
        const QPointF& tan_out_rel = {0, 0}
    )
    {
        return {pos, pos+tan_in_rel, pos+tan_out_rel};
    }
};

} // namespace math
