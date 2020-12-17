#include "group.hpp"

#include <QPainter>

#include "model/document.hpp"

GLAXNIMATE_OBJECT_IMPL(model::Group)


model::Group::Group(Document* document)
    : ShapeElement(document)
{
    connect(transform.get(), &Object::property_changed,
            this, &Group::on_transform_matrix_changed);
}

void model::Group::on_paint(QPainter* painter, model::FrameTime time, model::DocumentNode::PaintMode) const
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

void model::Group::add_shapes(model::FrameTime t, math::bezier::MultiBezier & bez) const
{
    for ( const auto& ch : utils::Range(shapes.begin(), shapes.past_first_modifier()) )
    {
        ch->add_shapes(t, bez);
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

QPainterPath model::Group::to_local_clip(FrameTime t) const
{
    return ShapeElement::shapes(t).painter_path();
}
