#include "property.hpp"
#include "model/object.hpp"
#include "command/property_commands.hpp"

model::BaseProperty::BaseProperty(Object* object, const QString& name, PropertyTraits traits)
    : object_(object), name_(name), traits_(traits)
{
    object_->add_property(this);
}

void model::BaseProperty::value_changed()
{
    object_->property_value_changed(this, value());
}

bool model::BaseProperty::set_undoable ( const QVariant& val, bool commit )
{
    QVariant before = value();
    if ( !set_value(val) )
        return false;

    object_->push_command(new command::SetPropertyValue(this, before, val, commit));
    return true;
}
