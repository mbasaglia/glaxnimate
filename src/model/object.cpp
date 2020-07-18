#include "object.hpp"

#include <unordered_map>

#include "property.hpp"

class model::Object::Private
{
public:
    std::unordered_map<QString, BaseProperty*> props;
    std::vector<BaseProperty*> prop_order;
    std::list<UnknownProperty> unknowns;
};


model::Object::Object()
    : d(std::make_unique<model::Object::Private>())
{
}

model::Object::~Object() = default;


void model::Object::clone_into(model::Object* dest) const
{
    for ( BaseProperty* prop : d->prop_order )
        dest->set(prop->name(), prop->value());
}



void model::Object::property_value_changed(const QString& name, const QVariant& value)
{
    emit property_changed(name, value);
}

void model::Object::add_property(model::BaseProperty* prop)
{
    d->props[prop->name()] = prop;
    d->prop_order.push_back(prop);
}

QVariant model::Object::get(const QString& property) const
{
    auto it = d->props.find(property);
    if ( it == d->props.end() )
         return QVariant{};
    return it->second->value();
}

bool model::Object::set(const QString& property, const QVariant& value, bool allow_unknown)
{
    auto it = d->props.find(property);
    if ( it == d->props.end() )
    {
        if ( allow_unknown )
        {
            d->unknowns.emplace_back(this, property, value);
            add_property(&d->unknowns.back());
            emit property_added(property, value);
            return true;
        }
        return false;
    }

    return it->second->set_value(value);
}

const std::vector<model::BaseProperty*>& model::Object::properties() const
{
    return d->prop_order;
}
