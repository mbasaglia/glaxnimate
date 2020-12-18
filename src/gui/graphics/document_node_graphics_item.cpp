#include "document_node_graphics_item.hpp"

#include "model/document.hpp"

graphics::DocumentNodeGraphicsItem::DocumentNodeGraphicsItem(model::DocumentNode* node, QGraphicsItem* parent)
    : QGraphicsObject(parent), node_(node)
{
//     setFlag(QGraphicsItem::ItemIsFocusable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemStacksBehindParent);
    setFlag(QGraphicsItem::ItemHasNoContents);

    /// \todo Setting
    setBoundingRegionGranularity(0);

    connect(node, &model::Object::property_changed, this, &DocumentNodeGraphicsItem::on_property_changed);
    connect(node, &model::DocumentNode::docnode_visible_recursive_changed, this, &DocumentNodeGraphicsItem::set_visible);
    set_visible(node->docnode_visible_recursive());
}

graphics::DocumentNodeGraphicsItem::~DocumentNodeGraphicsItem()
{}

void graphics::DocumentNodeGraphicsItem::shape_changed()
{
    prepareGeometryChange();
}

QRectF graphics::DocumentNodeGraphicsItem::boundingRect() const
{
    return node_->local_bounding_rect(node_->time());
}

void graphics::DocumentNodeGraphicsItem::paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*)
{
}

void graphics::DocumentNodeGraphicsItem::on_property_changed(const model::BaseProperty* prop)
{
    if ( prop->traits().flags & model::PropertyTraits::Visual )
    {
        prepareGeometryChange();
        update();
    }
}
