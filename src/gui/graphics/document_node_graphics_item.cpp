#include "document_node_graphics_item.hpp"

#include "model/document.hpp"

using namespace glaxnimate::gui;

graphics::DocumentNodeGraphicsItem::DocumentNodeGraphicsItem(model::VisualNode* node, QGraphicsItem* parent)
    : QGraphicsObject(parent), node_(node)
{
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemStacksBehindParent);
    setFlag(QGraphicsItem::ItemHasNoContents);

    setBoundingRegionGranularity(0);

    connect(node, &model::Object::visual_property_changed, this, &DocumentNodeGraphicsItem::shape_changed);
    connect(node, &model::VisualNode::docnode_visible_recursive_changed, this, &DocumentNodeGraphicsItem::set_visible);
    set_visible(node->docnode_visible_recursive());
}

graphics::DocumentNodeGraphicsItem::~DocumentNodeGraphicsItem()
{}

void graphics::DocumentNodeGraphicsItem::shape_changed()
{
//     prepareGeometryChange();
    rect_cache = {};
    cache_dirty = true;
}

QRectF graphics::DocumentNodeGraphicsItem::boundingRect() const
{
    if ( cache_dirty )
    {
        rect_cache = node_->local_bounding_rect(node_->time());
        cache_dirty = false;
    }
    return rect_cache;
}

void graphics::DocumentNodeGraphicsItem::paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*)
{
}
