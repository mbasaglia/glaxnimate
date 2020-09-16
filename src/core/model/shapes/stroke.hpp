#pragma once

#include <set>

#include <QBrush>
#include <QPainter>

#include "shape.hpp"
#include "model/animation/animatable.hpp"

namespace model {

class Stroke : public ObjectBase<Stroke, Styler>
{
    GLAXNIMATE_OBJECT

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


    GLAXNIMATE_ANIMATABLE(QColor, color, QColor())
    GLAXNIMATE_ANIMATABLE(float, width, 1)
    GLAXNIMATE_PROPERTY(Cap, cap, RoundCap, nullptr, nullptr, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(Join, join, RoundJoin, nullptr, nullptr, PropertyTraits::Visual)
    GLAXNIMATE_ANIMATABLE(float, opacity, 1)
    GLAXNIMATE_PROPERTY(float, miter_limit, 0, nullptr, nullptr, PropertyTraits::Visual)

public:
    using Ctor::Ctor;

    QRectF local_bounding_rect(FrameTime t) const override
    {
        qreal half_width = width.get_at(t) / 2;
        return collect_shapes(t).bounding_box().adjusted(
            -half_width, -half_width, half_width, half_width
        );
    }

    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("format-stroke-color");
    }

    QString type_name_human() const override
    {
        return tr("Stroke");
    }

protected:
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

    QBrush brush(FrameTime t) const
    {
        return color.get_at(t);
    }
};

} // namespace model
