#include "object.hpp"

#include <unordered_map>

#include "property.hpp"
#include "model/document.hpp"
#include "command/property_commands.hpp"


#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
namespace std {
  template<> struct hash<QString> {
    std::size_t operator()(const QString& s) const noexcept {
      return (size_t) qHash(s);
    }
  };
}
#endif

class model::Object::Private
{
public:
    std::unordered_map<QString, BaseProperty*> props;
    std::vector<BaseProperty*> prop_order;
    std::list<UnknownProperty> unknowns;
    Document* document;
};


model::Object::Object(Document* document)
    : d(std::make_unique<model::Object::Private>())
{
    d->document = document;
}

model::Object::~Object() = default;

void model::Object::assign_from(const model::Object* other)
{
    other->clone_into(this);
}

void model::Object::clone_into(model::Object* dest) const
{
    for ( BaseProperty* prop : d->prop_order )
        dest->set(prop->name(), prop->value());
}


void model::Object::property_value_changed(const BaseProperty* prop, const QVariant& value)
{
    on_property_changed(prop, value);
    emit property_changed(prop->name(), value);
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

model::BaseProperty * model::Object::get_property ( const QString& property )
{
    auto it = d->props.find(property);
    if ( it == d->props.end() )
        return nullptr;
    return it->second;
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

bool model::Object::has ( const QString& property ) const
{
    return d->props.find(property) != d->props.end();
}


const std::vector<model::BaseProperty*>& model::Object::properties() const
{
    return d->prop_order;
}

QString model::Object::type_name() const
{
    return naked_type_name(metaObject()->className());
}

QString model::Object::naked_type_name(QString class_name)
{
    int ns = class_name.lastIndexOf(":");
    if ( ns != -1 )
        class_name = class_name.mid(ns+1);
    return class_name;
}

model::Document * model::Object::document() const
{
    return d->document;
}

bool model::Object::set_undoable ( const QString& property, const QVariant& value )
{

    auto it = d->props.find(property);
    if ( it != d->props.end() )
        return it->second->set_undoable(value);
    return false;
}

bool model::BaseProperty::set_undoable ( const QVariant& val )
{
    QVariant before = value();
    if ( !set_value(val) )
        return false;

    object_->document()->undo_stack().push(new command::SetPropertyValue(this, before, val));
    return true;
}
