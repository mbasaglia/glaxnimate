#include "fill.hpp"

GLAXNIMATE_OBJECT_IMPL(model::Fill)

void model::Fill::on_paint(QPainter* p, model::FrameTime t, model::VisualNode::PaintMode, model::Modifier* modifier) const
{
    p->setBrush(brush(t));
    p->setOpacity(p->opacity() * opacity.get_at(t));
    p->setPen(Qt::NoPen);

    math::bezier::MultiBezier bez;
    if ( modifier )
        bez = modifier->collect_shapes_from(affected(), t, {});
    else
        bez = collect_shapes(t, {});

    QPainterPath path = bez.painter_path();

    path.setFillRule(Qt::FillRule(fill_rule.get()));
    p->drawPath(path);
}

QPainterPath model::Fill::to_painter_path(model::FrameTime t) const
{
    return collect_shapes(t, {}).painter_path();
}

