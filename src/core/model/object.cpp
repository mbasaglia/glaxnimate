#include "object.hpp"

#include <unordered_map>

#include "property/property.hpp"
#include "model/document.hpp"
#include "app/log/log.hpp"


class model::Object::Private
{
public:
    std::unordered_map<QString, BaseProperty*> props;
    std::vector<BaseProperty*> prop_order;
    Document* document;
    FrameTime current_time = 0;
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

void model::Object::transfer(model::Document* document)
{
    d->document = document;
    for ( auto prop: d->prop_order )
        prop->transfer(document);
}


void model::Object::clone_into(model::Object* dest) const
{
    if ( dest->metaObject() != metaObject() )
    {
        app::log::Log log("Object", type_name());
        log.stream(app::log::Error) << "trying to clone into" << dest->type_name() << "from" << type_name();
        log.stream(app::log::Info) << "make sure clone_covariant is implemented for" << type_name() << "or use GLAXNIMATE_OBJECT";
        return;
    }

    for ( BaseProperty* prop : d->prop_order )
        dest->get_property(prop->name())->assign_from(prop);
}


void model::Object::property_value_changed(const BaseProperty* prop, const QVariant& value)
{
    on_property_changed(prop, value);
    emit property_changed(prop, value);
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

bool model::Object::set(const QString& property, const QVariant& value)
{
    auto it = d->props.find(property);
    if ( it == d->props.end() )
        return false;

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
    return detail::naked_type_name(metaObject()->className());
}

QString model::detail::naked_type_name(QString class_name)
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

void model::Object::push_command(QUndoCommand* cmd)
{
    d->document->push_command(cmd);
}


bool model::Object::set_undoable ( const QString& property, const QVariant& value )
{

    auto it = d->props.find(property);
    if ( it != d->props.end() )
        return it->second->set_undoable(value);
    return false;
}

void model::Object::set_time(model::FrameTime t)
{
    d->current_time = t;
    for ( auto prop: d->prop_order )
        prop->set_time(t);
}

model::FrameTime model::Object::time() const
{
    return d->current_time;
}
