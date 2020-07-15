#include "layers.hpp"

#include "composition.hpp"


void model::ChildLayerView::iterator::find_first()
{
    while ( index < comp->layers.size() && comp->layers[index].parent.get() != parent )
        ++index;
}

model::Layer & model::ChildLayerView::iterator::operator*() const
{
    return comp->layers[index];
}

model::Layer * model::ChildLayerView::iterator::operator->() const
{
    return &comp->layers[index];
}

