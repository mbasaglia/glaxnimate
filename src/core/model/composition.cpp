#include "composition.hpp"


model::Composition::Composition(model::Document* document)
    : DocumentNode(document)
{
}

int model::Composition::docnode_child_index(model::DocumentNode* dn) const
{
    return shapes.index_of(static_cast<ShapeElement*>(dn));
}
