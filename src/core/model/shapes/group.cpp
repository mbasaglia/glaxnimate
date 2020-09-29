#include "group.hpp"

#include <QPainter>

GLAXNIMATE_OBJECT_IMPL(model::Group)


void model::Group::on_paint(QPainter* painter, model::FrameTime time, model::DocumentNode::PaintMode) const
{
    painter->setOpacity(
        painter->opacity() * opacity.get_at(time)
    );
}
