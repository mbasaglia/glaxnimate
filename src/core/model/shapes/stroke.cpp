#include "stroke.hpp"
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Stroke)

void glaxnimate::model::Stroke::on_paint(QPainter* p, glaxnimate::model::FrameTime t, glaxnimate::model::VisualNode::PaintMode, glaxnimate::model::Modifier* modifier) const
{
    QPen pen(brush(t), width.get_at(t));
    pen.setCapStyle(Qt::PenCapStyle(cap.get()));
    pen.setJoinStyle(Qt::PenJoinStyle(join.get()));
    pen.setMiterLimit(miter_limit.get());
    p->setBrush(Qt::NoBrush);
    p->setPen(pen);
    p->setOpacity(p->opacity() * opacity.get_at(t));

    math::bezier::MultiBezier bez;
    if ( modifier )
        bez = modifier->collect_shapes_from(affected(), t, {});
    else
        bez = collect_shapes(t, {});

    p->drawPath(bez.painter_path());
}

void glaxnimate::model::Stroke::set_pen_style ( const QPen& pen_style )
{
    color.set(pen_style.color());
    width.set(pen_style.width());
    cap.set(glaxnimate::model::Stroke::Cap(pen_style.capStyle()));
    join.set(glaxnimate::model::Stroke::Join(pen_style.joinStyle()));
    miter_limit.set(pen_style.miterLimit());
}

void glaxnimate::model::Stroke::set_pen_style_undoable(const QPen& pen_style)
{
    color.set_undoable(pen_style.color());
    width.set_undoable(pen_style.width());
    cap.set_undoable(QVariant::fromValue(glaxnimate::model::Stroke::Cap(pen_style.capStyle())));
    join.set_undoable(QVariant::fromValue(glaxnimate::model::Stroke::Join(pen_style.joinStyle())));
    miter_limit.set_undoable(pen_style.miterLimit());
}

QPainterPath glaxnimate::model::Stroke::to_painter_path(glaxnimate::model::FrameTime t) const
{
    QPainterPathStroker s;
    s.setWidth(width.get_at(t));
    s.setCapStyle(Qt::PenCapStyle(cap.get()));
    s.setJoinStyle(Qt::PenJoinStyle(join.get()));
    s.setMiterLimit(miter_limit.get());
    return s.createStroke(collect_shapes(t, {}).painter_path());
}
