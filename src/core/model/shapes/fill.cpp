#include "fill.hpp"

GLAXNIMATE_OBJECT_IMPL(model::Fill)

void model::Fill::on_paint(QPainter* p, model::FrameTime t, model::DocumentNode::PaintMode) const
{
    p->setBrush(brush(t));
    p->setOpacity(p->opacity() * opacity.get_at(t));
    p->setPen(Qt::NoPen);
    QPainterPath path = collect_shapes(t).painter_path();
    path.setFillRule(Qt::FillRule(fill_rule.get()));
    p->drawPath(path);
}
