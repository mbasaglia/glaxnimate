#pragma once

#include <QGraphicsObject>
#include <QBrush>
#include <QPixmap>
#include <QPainter>

#include "model/animation.hpp"
#include "handle.hpp"
#include "model/graphics/document_node_graphics_item.hpp"
#include "command/property_commands.hpp"
#include "model/document.hpp"
#include "app/application.hpp"

namespace model::graphics {

class AnimationItem : public DocumentNodeGraphicsItem
{
public:
    explicit AnimationItem(Animation* animation)
        : animation(animation)
    {
        connect(animation, &Object::property_changed, this, &AnimationItem::on_property_changed);

        handle_h = new MoveHandle(this, MoveHandle::Horizontal, MoveHandle::Diamond, 8);
        handle_v = new MoveHandle(this, MoveHandle::Vertical, MoveHandle::Diamond, 8);
        handle_hv = new MoveHandle(this, MoveHandle::DiagonalDown, MoveHandle::Square);
        connect(handle_h, &MoveHandle::dragged_x, this, &AnimationItem::dragged_x);
        connect(handle_v, &MoveHandle::dragged_y, this, &AnimationItem::dragged_y);
        connect(handle_hv, &MoveHandle::dragged, this, &AnimationItem::dragged_xy);
        connect(handle_h,  &MoveHandle::drag_finished, this, &AnimationItem::drag_finished);
        connect(handle_v,  &MoveHandle::drag_finished, this, &AnimationItem::drag_finished);
        connect(handle_hv, &MoveHandle::drag_finished, this, &AnimationItem::drag_finished);

        back.setTexture(QPixmap(app::Application::instance()->data_file("images/widgets/background.png")));

        update_handles();
    }

    QRectF boundingRect() const override
    {
        return QRectF(0, 0, animation->width.get(), animation->height.get());
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        painter->fillRect(boundingRect(), back);
        animation->paint(painter, animation->document()->current_time(), true);
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

    void dragged_xy(const QPointF& pos)
    {
        update_size(pos.x(), pos.y(), false);
    }

    void dragged_x(qreal x)
    {
        update_size(x, animation->height.get(), false);
    }

    void dragged_y(qreal y)
    {
        update_size(animation->width.get(), y, false);
    }

    void drag_finished()
    {
        update_size(animation->width.get(), animation->height.get(), true);
    }


private:
    void update_size(qreal x, qreal y, bool commit)
    {
        int w = std::max(1, qRound(x));
        int h = std::max(1, qRound(y));
        if ( w != animation->width.get() || h != animation->height.get() )
        {
            animation->document()->undo_stack().push(new command::SetMultipleProperties(
                QObject::tr("Resize Animation Canvas"),
                commit,
                {&animation->width, &animation->height},
                w, h
            ));
        };
        update_handles();
    }

    void update_handles()
    {
        handle_h->setPos(QPointF(animation->width.get(), animation->height.get() / 2.0));
        handle_v->setPos(QPointF(animation->width.get() / 2.0, animation->height.get()));
        handle_hv->setPos(QPointF(animation->width.get(), animation->height.get()));
    }

private:
    Animation* animation;
    QBrush back;
    MoveHandle* handle_h;
    MoveHandle* handle_v;
    MoveHandle* handle_hv;
};

} // namespace model::graphics
