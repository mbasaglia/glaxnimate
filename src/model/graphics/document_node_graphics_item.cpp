#include "document_node_graphics_item.hpp"

#include "model/document.hpp"

QRectF model::graphics::DocumentNodeGraphicsItem::boundingRect() const
{
    return node->local_bounding_rect(node->document()->current_time());
}

void model::graphics::DocumentNodeGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    node->paint(painter, node->document()->current_time(), false);
}
