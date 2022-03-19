#pragma once

#include "glaxnimate/core/model/property/object_list_property.hpp"
#include "glaxnimate/core/model/shapes/layer.hpp"

namespace glaxnimate::model {

class Composition : public VisualNode
{
    Q_OBJECT

    GLAXNIMATE_PROPERTY_LIST(model::ShapeElement, shapes)

public:
    using VisualNode::VisualNode;

    utils::Range<Layer::ChildLayerIterator> top_level() const
    {
        return {
            Layer::ChildLayerIterator(&shapes, nullptr, 0),
            Layer::ChildLayerIterator(&shapes, nullptr, shapes.size())
        };
    }

    DocumentNode* docnode_child(int index) const override
    {
        return shapes[index];
    }

    int docnode_child_count() const override
    {
        return shapes.size();
    }

    int docnode_child_index(DocumentNode* dn) const override;

    QRectF local_bounding_rect(FrameTime t) const override;
};

} // namespace glaxnimate::model

