#pragma once

#include "model/shapes/rect.hpp"
#include "model/document.hpp"
#include "command/animation_commands.hpp"
#include "handle.hpp"

namespace graphics {

class RectRounder : public QGraphicsObject
{
public:
    RectRounder(model::Rect* target)
        : target(target),
        handle(this, MoveHandle::Any, MoveHandle::Circle)
    {
        update_pos();
        connect(&handle, &MoveHandle::dragged, this, &RectRounder::on_drag);
        connect(&handle, &MoveHandle::drag_finished, this, &RectRounder::on_commit);
        connect(target, &model::Rect::property_changed, this, &RectRounder::on_prop_changed);
        handle.set_associated_property(&target->rounded);
    }

    QRectF boundingRect() const override { return {}; }
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override {}

private slots:
    void on_drag(const QPointF& p)
    {
        handle.setPos(restrict(p));
        push(false);
    }

    void on_commit()
    {
        push(true);
    }

    void on_prop_changed(const model::BaseProperty* prop)
    {
        if ( prop->traits().flags & model::PropertyTraits::Visual )
            update_pos();
    }

private:
    QPointF restrict(const QPointF& p)
    {
        qreal bound_y = qBound(itemrect.top(), p.y(), itemrect.center().y());
        qreal round_y = bound_y - itemrect.top();
        if ( round_y > itemrect.width() / 2 )
            bound_y = itemrect.top() + itemrect.width() / 2;

        return QPointF(itemrect.right(), bound_y);
    }

    void update_pos()
    {
        itemrect = target->local_bounding_rect(target->time());
        QPointF topright = itemrect.topRight();
        float round = target->rounded.get();
        handle.setPos(restrict(QPointF(topright.x(), topright.y()+round)));
    }

    void push(bool commit)
    {
        float val = handle.pos().y() - itemrect.top();
        target->document()->undo_stack().push(new command::SetMultipleAnimated(
            target->rounded.name(), {&target->rounded},
            {target->rounded.value()}, {val}, commit
        ));
    }

    QRectF itemrect;
    model::Rect* target;
    MoveHandle handle;
};

} // namespace graphics
