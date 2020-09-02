#include "shape.hpp"


const model::ShapeListProperty& model::ShapeElement::siblings() const
{
    return static_cast<const ShapeListProperty&>(
        *docnode_parent()->get_property("shapes")
    );
}

model::DocumentNode * model::ShapeElement::docnode_parent() const
{
    return static_cast<DocumentNode*>(siblings().object());
}
