#pragma once

#include "shape.hpp"
#include "math/ellipse_solver.hpp"

namespace model {


class Ellipse : public Shape
{
    GLAXNIMATE_OBJECT
    GLAXNIMATE_ANIMATABLE(QPointF, position, QPointF())
    GLAXNIMATE_ANIMATABLE(QSizeF, size, QSizeF())

public:
    using Shape::Shape;

    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("draw-ellipse");
    }

    QString type_name_human() const override
    {
        return tr("Ellipse");
    }

    math::Bezier to_bezier(FrameTime t) const override
    {
        QSizeF sz = size.get_at(t);
        return math::EllipseSolver(position.get_at(t), QPointF(sz.width()/2, sz.height()/2), 0).to_bezier(0, 2*M_PI);
    }

    QRectF local_bounding_rect(FrameTime t) const override
    {
        QSizeF sz = size.get_at(t);
        return QRectF(position.get_at(t) - QPointF(sz.width()/2, sz.height()/2), sz);
    }
};

} // namespace model
