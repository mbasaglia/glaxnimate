#include "object.hpp"

#include <unordered_map>

#include "property/property.hpp"
#include "model/document.hpp"
#include "app/log/log.hpp"


class glaxnimate::model::Object::Private
{
public:
    std::unordered_map<QString, BaseProperty*> props;
    std::vector<BaseProperty*> prop_order;
    Document* document;
    FrameTime current_time = 0;
};


glaxnimate::model::Object::Object(Document* document)
    : d(std::make_unique<glaxnimate::model::Object::Private>())
{
    d->document = document;
}

glaxnimate::model::Object::~Object() = default;

void glaxnimate::model::Object::assign_from(const glaxnimate::model::Object* other)
{
    other->clone_into(this);
}

void glaxnimate::model::Object::transfer(glaxnimate::model::Document* document)
{
    d->document = document;
    for ( auto prop: d->prop_order )
        prop->transfer(document);
}


void glaxnimate::model::Object::clone_into(glaxnimate::model::Object* dest) const
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


void glaxnimate::model::Object::property_value_changed(const BaseProperty* prop, const QVariant& value)
{
    on_property_changed(prop, value);
    emit property_changed(prop, value);
    if ( prop->traits().flags & PropertyTraits::Visual )
    {
        d->document->graphics_invalidated();
        emit visual_property_changed(prop, value);
    }
}

void glaxnimate::model::Object::add_property(glaxnimate::model::BaseProperty* prop)
{
    d->props[prop->name()] = prop;
    d->prop_order.push_back(prop);
}

QVariant glaxnimate::model::Object::get(const QString& property) const
{
    auto it = d->props.find(property);
    if ( it == d->props.end() )
         return QVariant{};
    return it->second->value();
}

glaxnimate::model::BaseProperty * glaxnimate::model::Object::get_property ( const QString& property )
{
    auto it = d->props.find(property);
    if ( it == d->props.end() )
        return nullptr;
    return it->second;
}

bool glaxnimate::model::Object::set(const QString& property, const QVariant& value)
{
    auto it = d->props.find(property);
    if ( it == d->props.end() )
        return false;

    return it->second->set_value(value);
}

bool glaxnimate::model::Object::has ( const QString& property ) const
{
    return d->props.find(property) != d->props.end();
}


const std::vector<glaxnimate::model::BaseProperty*>& glaxnimate::model::Object::properties() const
{
    return d->prop_order;
}

QString glaxnimate::model::Object::type_name() const
{
    return detail::naked_type_name(metaObject()->className());
}

QString glaxnimate::model::detail::naked_type_name(QString class_name)
{
    int ns = class_name.lastIndexOf(":");
    if ( ns != -1 )
        class_name = class_name.mid(ns+1);
    return class_name;
}

glaxnimate::model::Document * glaxnimate::model::Object::document() const
{
    return d->document;
}

void glaxnimate::model::Object::push_command(QUndoCommand* cmd)
{
    d->document->push_command(cmd);
}


bool glaxnimate::model::Object::set_undoable ( const QString& property, const QVariant& value )
{

    auto it = d->props.find(property);
    if ( it != d->props.end() )
        return it->second->set_undoable(value);
    return false;
}

void glaxnimate::model::Object::set_time(glaxnimate::model::FrameTime t)
{
    d->current_time = t;
    for ( auto prop: d->prop_order )
        prop->set_time(t);
}

glaxnimate::model::FrameTime glaxnimate::model::Object::time() const
{
    return d->current_time;
}

void glaxnimate::model::Object::stretch_time(qreal multiplier)
{
    for ( const auto& prop : d->prop_order )
        prop->stretch_time(multiplier);

    d->current_time *= multiplier;
}
