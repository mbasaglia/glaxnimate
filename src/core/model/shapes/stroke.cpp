#include "stroke.hpp"

GLAXNIMATE_OBJECT_IMPL(model::Stroke)

void model::Stroke::on_paint(QPainter* p, model::FrameTime t, model::DocumentNode::PaintMode) const
{
    QPen pen(brush(t), width.get_at(t));
    pen.setCapStyle(Qt::PenCapStyle(cap.get()));
    pen.setJoinStyle(Qt::PenJoinStyle(join.get()));
    pen.setMiterLimit(miter_limit.get());
    p->setBrush(Qt::NoBrush);
    p->setPen(pen);
    p->setOpacity(p->opacity() * opacity.get_at(t));
    p->drawPath(collect_shapes(t).painter_path());
}

void model::Stroke::set_pen_style ( const QPen& pen_style )
{
    color.set(pen_style.color());
    width.set(pen_style.width());
    cap.set(model::Stroke::Cap(pen_style.capStyle()));
    join.set(model::Stroke::Join(pen_style.joinStyle()));
    miter_limit.set(pen_style.miterLimit());
}
