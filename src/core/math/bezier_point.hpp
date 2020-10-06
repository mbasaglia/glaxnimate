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

    void drag_tan_in(const QPointF& p)
    {
        tan_in = p;
        tan_out = drag_tangent(tan_in, tan_out, pos, type);
    }

    void drag_tan_out(const QPointF& p)
    {
        tan_out = p;
        tan_in = drag_tangent(tan_out, tan_in, pos, type);
    }

    void adjust_handles_from_type();

    void set_point_type(BezierPointType t)
    {
        type = t;
        adjust_handles_from_type();
    }

    static QPointF drag_tangent(const QPointF& dragged, const QPointF& other, const QPointF& pos, BezierPointType type)
    {
        if ( type == math::BezierPointType::Symmetrical )
        {
            return 2*pos - dragged;
        }
        else if ( type == math::BezierPointType::Smooth )
        {
            return math::PolarVector<QPointF>{
                math::length(other - pos),
                M_PI + math::angle(dragged - pos)
            }.to_cartesian() + pos;
        }

        return other;
    }

    void translate(const QPointF& delta)
    {
        pos += delta;
        tan_in += delta;
        tan_out += delta;
    }

    void translate_to(const QPointF& new_pos)
    {
        translate(new_pos - pos);
    }

};

} // namespace math
