#pragma once

#include "model/property/object_list_property.hpp"
#include "model/layers/layer.hpp"

namespace model {

class Composition : public DocumentNode
{
    Q_OBJECT

    GLAXNIMATE_SUBOBJECT(AnimationContainer, animation)
    GLAXNIMATE_PROPERTY_LIST(Layer, layers, &Composition::layer_added, &Composition::layer_removed, &DocumentNode::docnode_child_add_begin, &DocumentNode::docnode_child_remove_begin)

public:
    explicit Composition(Document* document);

    utils::Range<Layer::ChildLayerIterator> top_level() const
    {
        return {
            Layer::ChildLayerIterator(this, nullptr, 0),
            Layer::ChildLayerIterator(this, nullptr, layers.size())
        };
    }

    DocumentNode* docnode_child(int index) const override
    {
        return layers[index];
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

