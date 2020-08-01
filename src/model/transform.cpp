#include "transform.hpp"

namespace {

QTransform make_transform(
    const QPointF& ap,
    const QPointF& pos,
    double rot,
    QVector2D s
)
{
    QTransform trans;
    trans.translate(pos.x(), pos.y());
    trans.rotate(rot);
    trans.scale(s.x(), s.y());
    trans.translate(-ap.x(), -ap.y());
    return trans;
}

} // namespace

QTransform model::Transform::transform_matrix() const
{
    return make_transform(
        anchor_point.get(),
        position.get(),
        rotation.get(),
        scale.get()
    );
}

QTransform model::Transform::transform_matrix(FrameTime f) const
{
    return make_transform(
        anchor_point.get_at(f),
        position.get_at(f),
        rotation.get_at(f),
        scale.get_at(f)
    );
}

