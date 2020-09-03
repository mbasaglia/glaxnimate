#pragma once

#include "shape.hpp"
#include "model/animation/animatable.hpp"

#include <QBrush>
#include <QPainter>

namespace model {

class Fill : public Modifier
{
    Q_OBJECT

public:
    enum Rule
    {
        NonZero = 1,
        EvenOdd = 2,
    };

private:
    Q_ENUM(Rule);

    GLAXNIMATE_PROPERTY(Rule, fill_rule, NonZero)
    GLAXNIMATE_ANIMATABLE(float, opacity, 1)

public:
    using Modifier::Modifier;

    QRectF local_bounding_rect(FrameTime t) const override
    {
        return collect_shapes(t).bounding_box();
    }

protected:
    virtual QBrush brush(FrameTime t) const = 0;

    void on_paint(QPainter* p, FrameTime t, PaintMode) const override
    {
        p->setBrush(brush(t));
        p->setPen(Qt::NoPen);
        p->drawPath(collect_shapes(t).painter_path());
    }
};

class SolidFill : public Fill
{
    GLAXNIMATE_OBJECT
    GLAXNIMATE_ANIMATABLE(QColor, color, QColor())

public:
    using Fill::Fill;

    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("format-fill-color");
    }

    QString type_name_human() const override
    {
        return tr("Fill");
    }

protected:
    QBrush brush(FrameTime t) const override
    {
        return color.get_at(t);
    }
};

} // namespace model
