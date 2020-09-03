#include "document_node_graphics_item.hpp"

#include "model/document.hpp"

model::graphics::DocumentNodeGraphicsItem::DocumentNodeGraphicsItem(DocumentNode* node, QGraphicsItem* parent)
    : QGraphicsObject(parent), node_(node)
{
//     setFlag(QGraphicsItem::ItemIsFocusable);
//     setFlag(QGraphicsItem::ItemIsSelectable);

    /// \todo Setting
    setBoundingRegionGranularity(0.25);

    connect(node, &model::Object::property_changed, this, &DocumentNodeGraphicsItem::on_property_changed);
}

QRectF model::graphics::DocumentNodeGraphicsItem::boundingRect() const
{
    return node_->local_bounding_rect(node_->document()->current_time());
}

void model::graphics::DocumentNodeGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    node_->paint(painter, node_->document()->current_time(), DocumentNode::NoTransform);
}

void model::graphics::DocumentNodeGraphicsItem::on_property_changed(const model::BaseProperty* prop)
{
    if ( prop->traits().flags & model::PropertyTraits::Animated )
    {
        prepareGeometryChange();
    }
}
