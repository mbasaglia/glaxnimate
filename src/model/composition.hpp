#pragma once

#include "layers.hpp"

namespace model {

class Composition : public AnimationContainer
{
    Q_OBJECT

    GLAXNIMATE_PROPERTY_LIST(Layer, layers)

public:
    using AnimationContainer::AnimationContainer;

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

    int docnode_child_index(DocumentNode* dn) const override;

    void add_layer(std::unique_ptr<Layer> lay, int position);

    std::unique_ptr<Layer> remove_layer(const QUuid& uuid);

    int layer_position(Layer* layer, int not_found=0) const;

    template<class LayerT>
    std::unique_ptr<LayerT> make_layer()
    {
        return std::make_unique<LayerT>(document(), this);
    }

signals:
    void layer_added(Layer* layer);
    void layer_removed(Layer* layer);
};

} // namespace model

