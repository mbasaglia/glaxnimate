#include "composition.hpp"


model::Composition::Composition(model::Document* document)
    : DocumentNode(document)
{
    connect(this, &Composition::layer_added, this, &DocumentNode::docnode_child_add_end);
    connect(this, &Composition::layer_removed, this, &DocumentNode::docnode_child_remove_end);
}


void model::Composition::add_layer(std::unique_ptr<Layer> lay, int position)
{
    layers.insert(std::move(lay), position);
}

std::unique_ptr<model::Layer> model::Composition::remove_layer(const QUuid& uuid)
{
    for ( int i = 0; i < layers.size(); i++ )
    {
        if ( layers[i]->uuid.get() == uuid )
        {
            return layers.remove(i);
        }
    }

    return {};
}

int model::Composition::layer_position(model::Layer* layer, int not_found) const
{
    for ( int i = 0; i < layers.size(); i++ )
    {
        if ( layers[i] == layer )
            return i;
    }

    return not_found;
}

int model::Composition::docnode_child_index(model::DocumentNode* dn) const
{
    return layers.index_of(static_cast<Layer*>(dn));
}
