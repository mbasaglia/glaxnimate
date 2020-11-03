#include "factory.hpp"

model::Object* model::Factory::static_build(const QString& name, model::Document* doc)
{
    return instance().build(name, doc);
}
