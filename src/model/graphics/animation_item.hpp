#pragma once

#include <QGraphicsObject>
#include <QBrush>
#include <QPixmap>
#include <QPainter>

#include "model/animation.hpp"
#include "handle.hpp"

namespace model::graphics {

class AnimationItem : public QGraphicsObject
{
public:
    explicit AnimationItem(Animation* animation)
        : animation(animation)
    {
        connect(animation, &Object::property_changed, this, &AnimationItem::on_property_changed);

        handle_h = new MoveHandle(this, MoveHandle::Horizontal, MoveHandle::Diamond, 8);
        handle_v = new MoveHandle(this, MoveHandle::Vertical, MoveHandle::Diamond, 8);
        handle_hv = new MoveHandle(this, MoveHandle::Any, MoveHandle::Square);
        update_handles();
    }

    QRectF boundingRect() const override
    {
        return QRectF(0, 0, animation->width.get(), animation->height.get());
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        painter->fillRect(boundingRect(), back);
    }

private slots:
    void on_property_changed(const QString& name)
    {
        if ( name == "width" || name == "height" )
        {
            prepareGeometryChange();
            update_handles();
        }
    }

    void update_handles()
    {
        handle_h->setPos(QPointF(animation->width.get(), animation->height.get() / 2.0));
        handle_v->setPos(QPointF(animation->width.get() / 2.0, animation->height.get()));
        handle_hv->setPos(QPointF(animation->width.get(), animation->height.get()));
    }

private:
    Animation* animation;
    QBrush back{QPixmap(QStringLiteral(":/color_widgets/alphaback.png"))};
    MoveHandle* handle_h;
    MoveHandle* handle_v;
    MoveHandle* handle_hv;
};

} // namespace model::graphics
