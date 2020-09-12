#pragma once

#include "shape.hpp"
#include "model/animation/animatable.hpp"

#include <QBrush>
#include <QPainter>

namespace model {

class Fill : public ObjectBase<Fill, Styler>
{
    GLAXNIMATE_OBJECT

public:
    enum Rule
    {
        NonZero = 1,
        EvenOdd = 2,
    };

private:
    Q_ENUM(Rule);

    GLAXNIMATE_ANIMATABLE(QColor, color, QColor())
    GLAXNIMATE_PROPERTY(Rule, fill_rule, NonZero, nullptr, nullptr, PropertyTraits::Visual)
    GLAXNIMATE_ANIMATABLE(float, opacity, 1)

public:
    using Ctor::Ctor;

    QRectF local_bounding_rect(FrameTime t) const override
    {
        return collect_shapes(t).bounding_box();
    }
    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("format-fill-color");
    }

    QString type_name_human() const override
    {
        return tr("Fill");
    }


protected:
    QBrush brush(FrameTime t) const
    {
        return color.get_at(t);
    }

    void on_paint(QPainter* p, FrameTime t, PaintMode) const override
    {
        p->setBrush(brush(t));
        p->setPen(Qt::NoPen);
        p->drawPath(collect_shapes(t).painter_path());
    }
};


} // namespace model
