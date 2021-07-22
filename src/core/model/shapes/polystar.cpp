#include "polystar.hpp"
#include "math/math.hpp"


GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::PolyStar)

glaxnimate::math::bezier::Bezier glaxnimate::model::PolyStar::to_bezier(model::FrameTime t) const
{
    return draw(type.get(), position.get_at(t), inner_radius.get_at(t), outer_radius.get_at(t), math::deg2rad(angle.get_at(t)), points.get_at(t));
}

glaxnimate::math::bezier::Bezier glaxnimate::model::PolyStar::draw(model::PolyStar::StarType type, const QPointF& pos, float r1, float r2, float angle_radians, int p)
{
    math::bezier::Bezier bezier;
    bezier.close();

    qreal halfd = math::pi / p;


    for ( int i = 0; i < p; i++ )
    {
        qreal main_angle = -math::pi / 2 + angle_radians + i * halfd * 2;
        qreal dx = r2 * math::cos(main_angle);
        qreal dy = r2 * math::sin(main_angle);
        bezier.add_point(QPointF(pos.x() + dx, pos.y() + dy));

        if ( type == Star )
        {
            dx = r1 * math::cos(main_angle+halfd);
            dy = r1 * math::sin(main_angle+halfd);
            bezier.add_point(QPointF(pos.x() + dx, pos.y() + dy));
        }
    }

    return bezier;
}


