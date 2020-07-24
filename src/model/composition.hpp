#pragma once

#include "layers.hpp"

namespace model {

class Composition : public DocumentNode
{
    Q_OBJECT

    GLAXNIMATE_PROPERTY_LIST(Layer, layers)

public:
    using DocumentNode::DocumentNode;

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

    void add_layer(std::unique_ptr<Layer> lay, int position);

    std::unique_ptr<Layer> remove_layer(const QUuid& uuid);

    int layer_position(Layer* layer, int not_found=0) const;

    template<class LayerT>
    std::unique_ptr<LayerT> make_layer()
    {
        return std::make_unique<LayerT>(document(), this);
    }

private:
    int layer_index = 0;
};

} // namespace model

