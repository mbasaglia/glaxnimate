#include "group.hpp"

#include <QPainter>

GLAXNIMATE_OBJECT_IMPL(model::Group)


void model::Group::on_paint(QPainter* painter, model::FrameTime time, model::DocumentNode::PaintMode) const
{
    painter->setOpacity(
        painter->opacity() * opacity.get_at(time)
    );
}

void model::Group::on_transform_matrix_changed()
{
    emit local_transform_matrix_changed(local_transform_matrix(time()));
    propagate_transform_matrix_changed(transform_matrix(time()), group_transform_matrix(time()));
}
