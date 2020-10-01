#pragma once
#include "document_node_graphics_item.hpp"

#include "model/shapes/shape.hpp"

namespace graphics {

class ShapeGraphicsItem : public DocumentNodeGraphicsItem
{
public:
    explicit ShapeGraphicsItem(model::ShapeElement* node, QGraphicsItem* parent = nullptr)
        : DocumentNodeGraphicsItem(node, parent)
    {
        setBoundingRegionGranularity(1);
    }

    QPainterPath shape() const override
    {
        return shape_element()->shapes(shape_element()->time()).painter_path();
    }

    model::ShapeElement* shape_element() const
    {
        return static_cast<model::ShapeElement*>(node());
    }
};

} // namespace graphics