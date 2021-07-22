#include "composition.hpp"


int glaxnimate::model::Composition::docnode_child_index(glaxnimate::model::DocumentNode* dn) const
{
    return shapes.index_of(static_cast<ShapeElement*>(dn));
}
