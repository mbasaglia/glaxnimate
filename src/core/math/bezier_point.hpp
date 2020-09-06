#pragma once

#include <QPointF>
#include "math/vector.hpp"

namespace math {

enum BezierPointType
{
    Corner,
    Smooth,
    Symmetrical,
};

struct BezierPoint
{
    QPointF pos;
    QPointF tan_in;
    QPointF tan_out;
    BezierPointType type;

    BezierPoint(
        const QPointF& pos,
        const QPointF& tan_in,
        const QPointF& tan_out,
        BezierPointType type = Corner
    ) : pos(pos), tan_in(tan_in), tan_out(tan_out), type(type)
    {}

    BezierPoint(const QPointF& pos)
        : BezierPoint(pos, pos, pos, Corner)
    {}

    static BezierPoint from_relative(
        const QPointF& pos,
        const QPointF& tan_in_rel = {0, 0},
        const QPointF& tan_out_rel = {0, 0},
        BezierPointType type = Corner
    )
    {
        return {pos, pos+tan_in_rel, pos+tan_out_rel, type};
    }

    QPointF relative_tan_in() const
    {
        return tan_in - pos;
    }

    QPointF relative_tan_out() const
    {
        return tan_out - pos;
    }

    PolarVector<QPointF> polar_tan_in() const
    {
        return relative_tan_in();
    }

    PolarVector<QPointF> polar_tan_out() const
    {
        return relative_tan_out();
    }
};

} // namespace math
