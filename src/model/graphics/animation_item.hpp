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
#include "model/graphics/transform_graphics_item.hpp"

namespace model::graphics {

class AnimationItem : public DocumentNodeGraphicsItem
{
public:
    explicit AnimationItem(Animation* animation)
        : DocumentNodeGraphicsItem(animation), animation(animation)
    {
        connect(animation, &Animation::width_changed, this, &AnimationItem::size_changed);
        connect(animation, &Animation::height_changed, this, &AnimationItem::size_changed);
        back.setTexture(QPixmap(app::Application::instance()->data_file("images/widgets/background.png")));
        setFlag(QGraphicsItem::ItemIsSelectable, false);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        painter->fillRect(boundingRect(), back);
    }

private slots:
    void size_changed()
    {
        prepareGeometryChange();
    }

private:
    Animation* animation;
    QBrush back;
};

class AnimationTransformItem : public QGraphicsObject
{
public:
    explicit AnimationTransformItem(Animation* animation)
        : animation(animation)
    {
        connect(animation, &Animation::width_changed, this, &AnimationTransformItem::size_changed);
        connect(animation, &Animation::height_changed, this, &AnimationTransformItem::size_changed);

        handle_h = new MoveHandle(this, MoveHandle::Horizontal, MoveHandle::Diamond, 8);
        handle_v = new MoveHandle(this, MoveHandle::Vertical, MoveHandle::Diamond, 8);
        handle_hv = new MoveHandle(this, MoveHandle::DiagonalDown, MoveHandle::Square);
        connect(handle_h, &MoveHandle::dragged_x, this, &AnimationTransformItem::dragged_x);
        connect(handle_v, &MoveHandle::dragged_y, this, &AnimationTransformItem::dragged_y);
        connect(handle_hv, &MoveHandle::dragged, this, &AnimationTransformItem::dragged_xy);
        connect(handle_h,  &MoveHandle::drag_finished, this, &AnimationTransformItem::drag_finished);
        connect(handle_v,  &MoveHandle::drag_finished, this, &AnimationTransformItem::drag_finished);
        connect(handle_hv, &MoveHandle::drag_finished, this, &AnimationTransformItem::drag_finished);

        update_handles();
    }

    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = nullptr) override {}
    QRectF boundingRect() const override { return {}; }

private slots:
    void size_changed()
    {
        update_handles();
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
    MoveHandle* handle_h;
    MoveHandle* handle_v;
    MoveHandle* handle_hv;
};


} // namespace model::graphics
