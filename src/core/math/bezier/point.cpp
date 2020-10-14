#include "point.hpp"


void math::bezier::Point::adjust_handles_from_type()
{
    if ( type != math::bezier::PointType::Corner )
    {
        math::PolarVector<QPointF> p_in(tan_in - pos);
        math::PolarVector<QPointF> p_out(tan_out - pos);

        qreal in_angle = (p_in.angle + p_out.angle + M_PI) / 2;
        if ( p_in.angle < p_out.angle )
            in_angle += M_PI;
        p_in.angle = in_angle;
        p_out.angle = in_angle + M_PI;

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
