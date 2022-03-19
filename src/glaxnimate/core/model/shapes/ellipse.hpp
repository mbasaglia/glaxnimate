#pragma once

#include "shape.hpp"
#include "glaxnimate/math/ellipse_solver.hpp"

namespace glaxnimate::model {


class Ellipse : public Shape
{
    GLAXNIMATE_OBJECT(Ellipse)
    GLAXNIMATE_ANIMATABLE(QPointF, position, QPointF())
    GLAXNIMATE_ANIMATABLE(QSizeF, size, QSizeF())

public:
    using Shape::Shape;

    QIcon tree_icon() const override;

    QString type_name_human() const override;

    math::bezier::Bezier to_bezier(FrameTime t) const override;

    QRectF local_bounding_rect(FrameTime t) const override;
};

} // namespace glaxnimate::model

