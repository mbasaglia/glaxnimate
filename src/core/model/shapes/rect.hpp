#pragma once

#include "shape.hpp"

namespace model {


class Rect : public ObjectBase<Rect, Shape>
{
    GLAXNIMATE_OBJECT
    GLAXNIMATE_ANIMATABLE(QPointF, position, QPointF())
    GLAXNIMATE_ANIMATABLE(QSizeF, size, QSizeF())
    GLAXNIMATE_ANIMATABLE(float, rounded, 0)

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

    math::Bezier to_bezier(FrameTime t) const override
    {
        math::Bezier bezier;
        QRectF bb = local_bounding_rect(t);
        float rounded = this->rounded.get_at(t);

        if ( rounded == 0 )
        {
            bezier.add_point(bb.topLeft());
            bezier.add_point(bb.topRight());
            bezier.add_point(bb.bottomRight());
            bezier.add_point(bb.bottomLeft());
        }
        else
        {
            QPointF hh(rounded/2, 0);
            QPointF vh(0, rounded/2);
            QPointF hd(rounded, 0);
            QPointF vd(0, rounded);
            bezier.add_point(bb.topLeft()+vd, {0,0}, -vh);
            bezier.add_point(bb.topLeft()+hd, -hh);
            bezier.add_point(bb.topRight()-hd, {0,0}, hh);
            bezier.add_point(bb.topRight()+vd, -vh);
            bezier.add_point(bb.bottomRight()-vd, {0,0}, vh);
            bezier.add_point(bb.bottomRight()-hd, hh);
            bezier.add_point(bb.bottomLeft()+hd, {0,0}, -hh);
            bezier.add_point(bb.bottomLeft()-vd, vh);
        }

        bezier.close();
        return bezier;
    }

    QRectF local_bounding_rect(FrameTime t) const override
    {
        QSizeF sz = size.get_at(t);
        return QRectF(position.get_at(t) - QPointF(sz.width()/2, sz.height()/2), sz);
    }
};

} // namespace model
