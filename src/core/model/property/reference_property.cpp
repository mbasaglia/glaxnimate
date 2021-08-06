#include "reference_property.hpp"
#include "model/document.hpp"
#include "model/animation/animatable_reference.hpp"

void glaxnimate::model::ReferenceBase::transfer(model::Document* doc)
{
    auto ref = get_ref();

    if ( ref && !is_valid_option(ref) )
        set_ref(doc->find_by_uuid(ref->uuid.get()));
}

glaxnimate::model::ReferenceBase* glaxnimate::model::ReferenceBase::cast(model::BaseProperty* prop)
{
    if ( prop->traits().type != PropertyTraits::ObjectReference )
        return nullptr;

    if ( prop->traits().flags & PropertyTraits::Animated )
        return static_cast<model::AnimatableReferenceBase*>(prop);


    return static_cast<model::ReferencePropertyBase*>(prop);
}
