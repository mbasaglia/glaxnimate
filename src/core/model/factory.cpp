#include "factory.hpp"

glaxnimate::model::Object* glaxnimate::model::Factory::static_build(const QString& name, glaxnimate::model::Document* doc)
{
    return instance().build(name, doc);
}
