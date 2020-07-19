#pragma once

#include "layers.hpp"

namespace model {

class Composition : public DocumentNode
{
    Q_OBJECT

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

    int add_layer(std::unique_ptr<Layer> lay, int position);

    std::unique_ptr<Layer> remove_layer(int index);

    int layer_position(Layer* layer, int not_found=0) const;

private:
    int layer_index = 0;
};

} // namespace model

