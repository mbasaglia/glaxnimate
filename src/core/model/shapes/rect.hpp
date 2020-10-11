#pragma once

#include "shape.hpp"

namespace model {


class Rect : public Shape
{
    GLAXNIMATE_OBJECT(Rect)
    GLAXNIMATE_ANIMATABLE(QPointF, position, QPointF())
    GLAXNIMATE_ANIMATABLE(QSizeF, size, QSizeF())
    GLAXNIMATE_ANIMATABLE(float, rounded, 0, {}, 0)

public:
    using Shape::Shape;

    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("draw-rectangle");
    }

    QString type_name_human() const override
    {
        return tr("Rectangle");
    }

    math::bezier::Bezier to_bezier(FrameTime t) const override;

    QRectF local_bounding_rect(FrameTime t) const override
    {
        QSizeF sz = size.get_at(t);
        return QRectF(position.get_at(t) - QPointF(sz.width()/2, sz.height()/2), sz);
    }
};

} // namespace model
