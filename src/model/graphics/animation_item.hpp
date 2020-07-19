#pragma once

#include <QGraphicsObject>
#include <QBrush>
#include <QPixmap>
#include <QPainter>

#include "model/animation.hpp"

namespace model::graphics {

class AnimationItem : public QGraphicsObject
{
public:
    explicit AnimationItem(Animation* animation)
        : animation(animation)
    {
        connect(animation, &Object::property_changed, this, &AnimationItem::on_property_changed);
    }

    QRectF boundingRect() const override
    {
        return QRectF(0, 0, animation->width.get(), animation->height.get());
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override
    {
        painter->setClipRect(boundingRect());
        painter->fillRect(boundingRect(), back);
    }

private slots:
    void on_property_changed(const QString& name, const QVariant& value)
    {
        if ( name == "width" || name == "height" )
            prepareGeometryChange();
    }

private:
    Animation* animation;
    QBrush back{QPixmap(QStringLiteral(":/color_widgets/alphaback.png"))};
};

} // namespace model::graphics
