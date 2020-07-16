#pragma once

#include "layers.hpp"

namespace model {

class Composition : public DocumentNode
{
public:
    ObjectListProperty<Layer> layers{this, "layers", "layers"};

    ChildLayerView top_level() const
    {
        return ChildLayerView(this, nullptr);
    }

    DocumentNode* docnode_child(int index) const override
    {
        return &layers[index];
    }

    DocumentNode* docnode_parent() const override
    {
        return nullptr;
    }

    int docnode_child_count() const override
    {
        return layers.size();
    }
};

} // namespace model

