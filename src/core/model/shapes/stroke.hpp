#pragma once

#include "shape.hpp"
#include "model/animation/animatable.hpp"

#include <QBrush>
#include <QPainter>

namespace model {

class BaseStroke : public Modifier
{
    Q_OBJECT

public:
    enum Cap
    {
        ButtCap = Qt::FlatCap,
        RoundCap = Qt::RoundCap,
        SquareCap = Qt::SquareCap,
    };

    enum Join
    {
        MiterJoin = Qt::MiterJoin,
        RoundJoin = Qt::RoundJoin,
        BevelJoin = Qt::BevelJoin,
    };

private:
    Q_ENUM(Cap);
    Q_ENUM(Join);

    GLAXNIMATE_PROPERTY(Cap, cap, RoundCap)
    GLAXNIMATE_PROPERTY(Join, join, RoundJoin)
    GLAXNIMATE_ANIMATABLE(float, width, 1)
    GLAXNIMATE_ANIMATABLE(float, opacity, 1)
    GLAXNIMATE_PROPERTY(float, miter_limit, 0)

public:
    using Modifier::Modifier;

    QRectF local_bounding_rect(FrameTime t) const override
    {
        qreal half_width = width.get_at(t) / 2;
        return collect_shapes(t).bounding_box().adjusted(
            -half_width, -half_width, half_width, half_width
        );
    }

protected:
    virtual QBrush brush(FrameTime t) const = 0;

    void on_paint(QPainter* p, FrameTime t, PaintMode) const override
    {
        QPen pen(brush(t), width.get_at(t));
        pen.setCapStyle(Qt::PenCapStyle(cap.get()));
        pen.setJoinStyle(Qt::PenJoinStyle(join.get()));
        pen.setMiterLimit(miter_limit.get());
        p->setBrush(Qt::NoBrush);
        p->setPen(pen);
        p->drawPath(collect_shapes(t).painter_path());
    }
};

class Stroke : public ObjectBase<Stroke, BaseStroke>
{
    GLAXNIMATE_OBJECT
    GLAXNIMATE_ANIMATABLE(QColor, color, QColor())

public:
    using Ctor::Ctor;

    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("format-stroke-color");
    }

    QString type_name_human() const override
    {
        return tr("Stroke");
    }

protected:
    QBrush brush(FrameTime t) const override
    {
        return color.get_at(t);
    }
};

} // namespace model

