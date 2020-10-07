#include "reference_target.hpp"
#include "model/property/sub_object_property.hpp"
#include "model/property/object_list_property.hpp"

void model::ReferenceTarget::refresh_uuid()
{
    uuid.set_value(QUuid::createUuid());
    for ( auto prop : properties() )
    {
        if ( prop->traits().type == PropertyTraits::Object )
        {
            if ( prop->traits().flags & PropertyTraits::List )
            {
                for ( auto v : prop->value().toList() )
                {
                    if ( auto obj = v.value<model::ReferenceTarget*>() )
                        obj->refresh_uuid();
                }
            }
            else
            {
                if ( auto obj = qobject_cast<ReferenceTarget*>(static_cast<model::SubObjectPropertyBase*>(prop)->sub_object()) )
                    obj->refresh_uuid();
            }
        }
    }
}

