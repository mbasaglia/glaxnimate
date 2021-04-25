#pragma once

#include <QStyleOptionGraphicsItem>
#include <QPainter>

#include "math/geom.hpp"
#include "math/vector.hpp"
#include "model/document.hpp"
#include "model/shapes/text.hpp"
#include "command/animation_commands.hpp"

#include "handle.hpp"


#include <QDebug>
namespace graphics {

class TextAttributesEditor : public QGraphicsObject
{
public:
    TextAttributesEditor(model::TextShape* shape) : shape(shape),
        handle_baseline(this, MoveHandle::Any, MoveHandle::Square),
        handle_size(this, MoveHandle::Vertical, MoveHandle::Circle),
        handle_line_gap(this, MoveHandle::Vertical, MoveHandle::Diamond)
    {
        connect(&handle_baseline, &MoveHandle::dragged, this, &TextAttributesEditor::on_drag_baseline);
        connect(&handle_baseline, &MoveHandle::drag_finished, this, &TextAttributesEditor::on_commit_baseline);
        handle_baseline.set_associated_properties({&shape->position});

        connect(&handle_size, &MoveHandle::dragged, this, &TextAttributesEditor::on_drag_size);
        connect(&handle_size, &MoveHandle::drag_finished, this, &TextAttributesEditor::on_commit_size);

        connect(&handle_line_gap, &MoveHandle::dragged, this, &TextAttributesEditor::on_drag_line_gap);
        connect(&handle_line_gap, &MoveHandle::drag_finished, this, &TextAttributesEditor::on_commit_line_gap);

        connect(shape, &model::Object::property_changed, this, &TextAttributesEditor::on_prop_changed);
        connect(shape->font.get(), &model::Object::property_changed, this, &TextAttributesEditor::on_prop_changed);

        reset_handles();
    }

    QRectF boundingRect() const override
    {
        return shape->local_bounding_rect(shape->time());
    }


    void paint(QPainter* painter, const QStyleOptionGraphicsItem * opt, QWidget *) override
    {
        painter->save();
        QPen pen(opt->palette.color(QPalette::Highlight), 1);
        pen.setCosmetic(true);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        auto base = handle_baseline.pos();
        auto rect = shape->local_bounding_rect(shape->time());
        painter->drawLine(base, base + QPointF(rect.width(), 0));
        painter->restore();
    }

private slots:
    void on_drag_baseline(QPointF p)
    {
        shape->position.set_undoable(p, false);
    }

    void on_commit_baseline()
    {
        shape->position.set_undoable(shape->position.value(), true);
    }

    void on_drag_size(QPointF p)
    {
        shape->font->size.set_undoable(math::max(1., math::length(shape->position.get() - p)), false);
    }

    void on_commit_size()
    {
        shape->font->size.set_undoable(shape->font->size.value(), true);
    }


    void on_drag_line_gap(QPointF p)
    {
        qreal spacing = shape->font->line_spacing_unscaled();
        if ( spacing == 0 )
            return;
        qreal distance = math::length(shape->position.get() - p);
        qreal relative = distance / spacing;
        shape->font->line_height.set_undoable(relative, false);
    }

    void on_commit_line_gap()
    {
        shape->font->line_height.set_undoable(shape->font->line_height.value(), true);
    }

    void on_prop_changed(const model::BaseProperty* prop)
    {
        reset_handles();
    }

private:
    void reset_handles()
    {
        QPointF pos = shape->position.get();
        handle_baseline.setPos(pos);
        handle_size.setPos(pos + QPointF(0, -shape->font->size.get()));
        handle_line_gap.setPos(pos + QPointF(0, shape->font->line_spacing()));
    }

    model::TextShape* shape;
    MoveHandle handle_baseline;
    MoveHandle handle_size;
    MoveHandle handle_line_gap;
};

} // namespace graphics
