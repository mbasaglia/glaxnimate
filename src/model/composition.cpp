#include "composition.hpp"


int model::Composition::add_layer(std::unique_ptr<Layer> lay, int position)
{
    emit docnode_child_add_begin(position);
    if ( lay->index.is_null() )
        lay->index.set(layer_index++);
    Layer* ptr = lay.get();
    layers.insert(std::move(lay), position);
    emit docnode_child_add_end(ptr);
    return ptr->index.get();
}

std::unique_ptr<model::Layer> model::Composition::remove_layer(int index)
{
    for ( int i = 0; i < layers.size(); i++ )
    {
        if ( layers[i].index.get() == index )
        {
            emit docnode_child_remove_begin(i);
            auto ptr = layers.remove(i);
            emit docnode_child_remove_end(ptr.get());
            return ptr;
        }
    }

    return {};
}
