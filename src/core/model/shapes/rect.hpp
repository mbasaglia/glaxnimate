#pragma once

#include "shape.hpp"

namespace model {


class Rect : public ObjectBase<Rect, Shape>
{
    GLAXNIMATE_OBJECT
    GLAXNIMATE_ANIMATABLE(QPointF, position, QPointF())
    GLAXNIMATE_ANIMATABLE(QSizeF, size, QSizeF())
    GLAXNIMATE_ANIMATABLE(float, rounded, 0, {}, 0)

public:
    using Ctor::Ctor;

    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("draw-rectangle");
    }

    QString type_name_human() const override
    {
        return tr("Rectangle");
    }

    math::Bezier to_bezier(FrameTime t) const override;

    QRectF local_bounding_rect(FrameTime t) const override
    {
        QSizeF sz = size.get_at(t);
        return QRectF(position.get_at(t) - QPointF(sz.width()/2, sz.height()/2), sz);
    }
};

} // namespace model
