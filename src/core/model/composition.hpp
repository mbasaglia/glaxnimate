#pragma once

#include "model/property/object_list_property.hpp"
#include "model/shapes/layer.hpp"

namespace model {

class Composition : public DocumentNode
{
    using Layer = Layer__new;
    Q_OBJECT

    GLAXNIMATE_SUBOBJECT(AnimationContainer, animation)

public:
    ShapeListProperty shapes{this, "shapes",
        &DocumentNode::docnode_child_add_end,
        &DocumentNode::docnode_child_remove_end,
        &DocumentNode::docnode_child_add_begin,
        &DocumentNode::docnode_child_remove_begin
    };

public:
    explicit Composition(Document* document);

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

    DocumentNode* docnode_parent() const override
    {
        return nullptr;
    }

    int docnode_child_count() const override
    {
        return shapes.size();
    }

    int docnode_child_index(DocumentNode* dn) const override;
};

} // namespace model

