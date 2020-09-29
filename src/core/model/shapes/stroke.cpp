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
