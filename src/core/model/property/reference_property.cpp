#include "reference_property.hpp"
#include "model/document.hpp"

void glaxnimate::model::ReferencePropertyBase::transfer(model::Document* doc)
{
    auto ref = get_ref();

    if ( ref && !is_valid_option(ref) )
        set_ref(doc->find_by_uuid(ref->uuid.get()));
}
