/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "point.hpp"

using namespace glaxnimate;


void math::bezier::Point::adjust_handles_from_type()
{
    if ( type != math::bezier::PointType::Corner )
    {
        math::PolarVector<QPointF> p_in(tan_in - pos);
        math::PolarVector<QPointF> p_out(tan_out - pos);

        qreal in_angle = (p_in.angle + p_out.angle + pi) / 2;
        if ( p_in.angle < p_out.angle )
            in_angle += pi;
        p_in.angle = in_angle;
        p_out.angle = in_angle + pi;

        if ( type == math::bezier::PointType::Symmetrical )
            p_in.length = p_out.length = (p_in.length + p_out.length) / 2;

        tan_in = p_in.to_cartesian() + pos;
        tan_out = p_out.to_cartesian() + pos;
    }
}

void math::bezier::Point::transform(const QTransform& t)
{
    pos = t.map(pos);
    tan_in = t.map(tan_in);
    tan_out = t.map(tan_out);
}
