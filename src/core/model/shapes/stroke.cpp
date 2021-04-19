#include "stroke.hpp"

GLAXNIMATE_OBJECT_IMPL(model::Stroke)

void model::Stroke::on_paint(QPainter* p, model::FrameTime t, model::VisualNode::PaintMode) const
{
    QPen pen(brush(t), width.get_at(t));
    pen.setCapStyle(Qt::PenCapStyle(cap.get()));
    pen.setJoinStyle(Qt::PenJoinStyle(join.get()));
    pen.setMiterLimit(miter_limit.get());
    p->setBrush(Qt::NoBrush);
    p->setPen(pen);
    p->setOpacity(p->opacity() * opacity.get_at(t));
    p->drawPath(collect_shapes(t, {}).painter_path());
}

void model::Stroke::set_pen_style ( const QPen& pen_style )
{
    color.set(pen_style.color());
    width.set(pen_style.width());
    cap.set(model::Stroke::Cap(pen_style.capStyle()));
    join.set(model::Stroke::Join(pen_style.joinStyle()));
    miter_limit.set(pen_style.miterLimit());
}

void model::Stroke::set_pen_style_undoable(const QPen& pen_style)
{
    color.set_undoable(pen_style.color());
    width.set_undoable(pen_style.width());
    cap.set_undoable(QVariant::fromValue(model::Stroke::Cap(pen_style.capStyle())));
    join.set_undoable(QVariant::fromValue(model::Stroke::Join(pen_style.joinStyle())));
    miter_limit.set_undoable(pen_style.miterLimit());
}

QPainterPath model::Stroke::to_painter_path(model::FrameTime t) const
{
    QPainterPathStroker s;
    s.setWidth(width.get());
    s.setCapStyle(Qt::PenCapStyle(cap.get()));
    s.setJoinStyle(Qt::PenJoinStyle(join.get()));
    s.setMiterLimit(miter_limit.get());
    return s.createStroke(collect_shapes(t, {}).painter_path());
}
