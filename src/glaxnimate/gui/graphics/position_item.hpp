#pragma once

#include "handle.hpp"
#include "glaxnimate/core/model/document.hpp"
#include "glaxnimate/core/command/animation_commands.hpp"

namespace glaxnimate::gui::graphics {

class PositionItem : public QGraphicsObject
{
public:
    PositionItem(model::AnimatedProperty<QPointF>* target)
        : target(target),
        handle(this, MoveHandle::Any, MoveHandle::Square)
    {
        handle.setPos(target->get());
        connect(&handle, &MoveHandle::dragged, this, &PositionItem::on_drag);
        connect(&handle, &MoveHandle::drag_finished, this, &PositionItem::on_commit);
        connect(target->object(), &model::Object::property_changed, this, &PositionItem::on_prop_changed);
        handle.set_associated_property(target);
    }

    QRectF boundingRect() const override { return {}; }
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override {}


private slots:
    void on_drag(const QPointF& p)
    {
        push(p, false);
    }

    void on_commit()
    {
        push(target->value(), true);
    }

    void on_prop_changed(const model::BaseProperty* prop)
    {
        if ( prop == target)
            handle.setPos(target->get());
    }

private:
    void push(const QVariant& val, bool commit)
    {
        target->object()->document()->undo_stack().push(new command::SetMultipleAnimated(
            target->name(), {target}, {target->value()}, {val}, commit
        ));
    }

    model::AnimatedProperty<QPointF>* target;
    MoveHandle handle;
};

} // namespace glaxnimate::gui::graphics
