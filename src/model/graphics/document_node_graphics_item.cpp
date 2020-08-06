#include "document_node_graphics_item.hpp"

#include "model/document.hpp"

model::graphics::DocumentNodeGraphicsItem::DocumentNodeGraphicsItem(DocumentNode* node, QGraphicsItem* parent)
    : QGraphicsObject(parent), node(node)
{
    setFlag(QGraphicsItem::ItemIsFocusable);
    setFlag(QGraphicsItem::ItemIsSelectable);
}

QRectF model::graphics::DocumentNodeGraphicsItem::boundingRect() const
{
    return node->local_bounding_rect(node->document()->current_time());
}

void model::graphics::DocumentNodeGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    node->paint(painter, node->document()->current_time(), false);
}
