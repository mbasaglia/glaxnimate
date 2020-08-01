#include "composition.hpp"


void model::Composition::add_layer(std::unique_ptr<Layer> lay, int position)
{
    emit docnode_child_add_begin(position);
    Layer* ptr = lay.get();
    layers.insert(std::move(lay), position);
    emit docnode_child_add_end(ptr);
    emit layer_added(ptr);
}

std::unique_ptr<model::Layer> model::Composition::remove_layer(const QUuid& uuid)
{
    for ( int i = 0; i < layers.size(); i++ )
    {
        if ( layers[i].uuid.get() == uuid )
        {
            emit docnode_child_remove_begin(i);
            auto ptr = layers.remove(i);
            emit docnode_child_remove_end(ptr.get());
            emit layer_removed(ptr.get());
            return ptr;
        }
    }

    return {};
}

int model::Composition::layer_position(model::Layer* layer, int not_found) const
{
    for ( int i = 0; i < layers.size(); i++ )
    {
        if ( &layers[i] == layer )
            return i;
    }

    return not_found;
}
