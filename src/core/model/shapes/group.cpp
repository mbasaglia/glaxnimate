#include "group.hpp"

#include <QPainter>

#include "model/document.hpp"

GLAXNIMATE_OBJECT_IMPL(model::Group)


model::Group::Group(Document* document)
    : Ctor(document)
{
    connect(transform.get(), &Object::property_changed,
            this, &Group::on_transform_matrix_changed);
}

void model::Group::on_paint(QPainter* painter, model::FrameTime time, model::VisualNode::PaintMode) const
{
    painter->setOpacity(
        painter->opacity() * opacity.get_at(time)
    );
}

void model::Group::on_transform_matrix_changed()
{
    emit bounding_rect_changed();
    emit local_transform_matrix_changed(local_transform_matrix(time()));
    propagate_transform_matrix_changed(transform_matrix(time()), group_transform_matrix(time()));
}

void model::Group::add_shapes(model::FrameTime t, math::bezier::MultiBezier & bez, const QTransform& parent_transform) const
{
    QTransform trans = transform.get()->transform_matrix(t) * parent_transform;
    for ( const auto& ch : utils::Range(shapes.begin(), shapes.past_first_modifier()) )
    {
        ch->add_shapes(t, bez, trans);
    }
}

QRectF model::Group::local_bounding_rect(FrameTime t) const
{
    if ( shapes.empty() )
        return QRectF(QPointF(0, 0), document()->size());
    return shapes.bounding_rect(t);
}

QTransform model::Group::local_transform_matrix(model::FrameTime t) const
{
    return transform.get()->transform_matrix(t);
}

QPainterPath model::Group::to_painter_path(model::FrameTime t) const
{
    QPainterPath path;
    for ( const auto& ch : utils::Range(shapes.begin(), shapes.past_first_modifier()) )
    {
        path.addPath(ch->to_clip(t));
    }
    return path;
}


QPainterPath model::Group::to_clip(FrameTime t) const
{
    return transform.get()->transform_matrix(t).map(to_painter_path(t));
}
