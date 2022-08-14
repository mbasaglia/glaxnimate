#include "fill.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Fill)

void glaxnimate::model::Fill::on_paint(QPainter* p, glaxnimate::model::FrameTime t, glaxnimate::model::VisualNode::PaintMode, glaxnimate::model::Modifier* modifier) const
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

QPainterPath glaxnimate::model::Fill::to_painter_path_impl(glaxnimate::model::FrameTime t) const
{
    return collect_shapes(t, {}).painter_path();
}

