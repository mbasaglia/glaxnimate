/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QPointF>
#include <QTransform>
#include "math/vector.hpp"
#include "math/math.hpp"

namespace glaxnimate::math::bezier {

enum PointType
{
    Corner,
    Smooth,
    Symmetrical,
};

struct Point
{
    QPointF pos;
    QPointF tan_in;
    QPointF tan_out;
    PointType type;

    Point(
        const QPointF& pos,
        const QPointF& tan_in,
        const QPointF& tan_out,
        PointType type = Corner
    ) : pos(pos), tan_in(tan_in), tan_out(tan_out), type(type)
    {}

    Point(const QPointF& pos = {0, 0})
        : Point(pos, pos, pos, Corner)
    {}

    static Point from_relative(
        const QPointF& pos,
        const QPointF& tan_in_rel = {0, 0},
        const QPointF& tan_out_rel = {0, 0},
        PointType type = Corner
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

    void set_point_type(PointType t)
    {
        type = t;
        adjust_handles_from_type();
    }

    static QPointF drag_tangent(const QPointF& dragged, const QPointF& other, const QPointF& pos, PointType type)
    {
        if ( type == math::bezier::PointType::Symmetrical )
        {
            return 2*pos - dragged;
        }
        else if ( type == math::bezier::PointType::Smooth )
        {
            return math::PolarVector<QPointF>{
                math::length(other - pos),
                pi + math::angle(dragged - pos)
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

    void transform(const QTransform& t);

    QPointF position() const { return pos; }
};


} // namespace glaxnimate::math::bezier

Q_DECLARE_METATYPE(glaxnimate::math::bezier::Point)
