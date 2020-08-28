#include "factory.hpp"

#include "layers.hpp"

model::Object * model::Factory::make_any(const QString& class_name, model::Document* document, model::Composition* comp) const
{
    if ( auto obj = make_object(class_name, document) )
        return obj;
    return make_layer(class_name, document, comp);
}
