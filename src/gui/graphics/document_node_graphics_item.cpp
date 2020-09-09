#include "document_node_graphics_item.hpp"

#include "model/document.hpp"

graphics::DocumentNodeGraphicsItem::DocumentNodeGraphicsItem(model::DocumentNode* node, QGraphicsItem* parent)
    : QGraphicsObject(parent), node_(node)
{
//     setFlag(QGraphicsItem::ItemIsFocusable);
//     setFlag(QGraphicsItem::ItemIsSelectable);

    /// \todo Setting
    setBoundingRegionGranularity(0.25);

    connect(node, &model::Object::property_changed, this, &DocumentNodeGraphicsItem::on_property_changed);
}

graphics::DocumentNodeGraphicsItem::~DocumentNodeGraphicsItem()
{}

void graphics::DocumentNodeGraphicsItem::shape_changed()
{
    prepareGeometryChange();
}

QRectF graphics::DocumentNodeGraphicsItem::boundingRect() const
{
    return node_->local_bounding_rect(node_->document()->current_time());
}

void graphics::DocumentNodeGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    node_->paint(painter, node_->document()->current_time(), model::DocumentNode::NoTransform);
}

void graphics::DocumentNodeGraphicsItem::on_property_changed(const model::BaseProperty* prop)
{
    if ( prop->traits().flags & model::PropertyTraits::Visual )
    {
        prepareGeometryChange();
        update();
    }
}
