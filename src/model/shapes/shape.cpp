#include "shape.hpp"


const model::ShapeListProperty& model::ShapeElement::siblings() const
{
    return *property_;
}

model::DocumentNode * model::ShapeElement::docnode_parent() const
{
    return static_cast<DocumentNode*>(property_->object());
}

model::ObjectListProperty<model::ShapeElement>::iterator model::ShapeListProperty::past_first_modifier() const
{
    auto it = std::find_if(begin(), end(), [](const pointer& p){
        return qobject_cast<Modifier*>(p.get());
    });
    if ( it != end() )
        ++it;
    return it;
}
