#include "polystar.hpp"
#include "math/math.hpp"


GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::PolyStar)

glaxnimate::math::bezier::Bezier glaxnimate::model::PolyStar::to_bezier(model::FrameTime t) const
{
    return draw(
        type.get(),
        position.get_at(t),
        inner_radius.get_at(t),
        outer_radius.get_at(t),
        math::deg2rad(angle.get_at(t)),
        points.get_at(t),
        inner_roundness.get_at(t),
        outer_roundness.get_at(t),
        reversed.get()
    );
}

glaxnimate::math::bezier::Bezier glaxnimate::model::PolyStar::draw(
    model::PolyStar::StarType type, const QPointF& pos,
    float radius_inner, float radius_outer, float angle_radians, int p,
    float round_inner, float round_outer,
    bool reverse
)
{
    math::bezier::Bezier bezier;
    bezier.close();

    qreal direction = reverse ? -1 : 1;
    qreal halfd = math::pi / p * direction;
    qreal tangent_len_outer = round_outer * (math::tau * radius_outer) / (p * 4) * direction;
    qreal tangent_len_inner = round_inner * (math::tau * radius_inner) / (p * 4) * direction;

    for ( int i = 0; i < p; i++ )
    {
        qreal main_angle = -math::pi / 2 + angle_radians + i * halfd * 2;
        qreal dx = radius_outer * math::cos(main_angle);
        qreal dy = radius_outer * math::sin(main_angle);

        QPointF tangents_outer;
        if ( radius_outer != 0 )
            tangents_outer = {
                dy / radius_outer,
                -dx / radius_outer,
            };

        bezier.add_point(
            QPointF(pos.x() + dx, pos.y() + dy),
            tangent_len_outer * tangents_outer,
            -tangent_len_outer * tangents_outer
        );

        if ( type == Star )
        {
            dx = radius_inner * math::cos(main_angle+halfd);
            dy = radius_inner * math::sin(main_angle+halfd);

            QPointF tangents_inner;
            if ( radius_inner != 0 )
                tangents_inner = {
                    dy / radius_inner,
                    -dx / radius_inner,
                };

            bezier.add_point(
                QPointF(pos.x() + dx, pos.y() + dy),
                tangent_len_inner * tangents_inner,
                -tangent_len_inner * tangents_inner
            );
        }
    }

    return bezier;
}


