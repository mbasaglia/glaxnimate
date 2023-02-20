/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#include "type_def.hpp"
#include "app/utils/qstring_hash.hpp"

namespace glaxnimate::io::rive {

class Object;

struct PropertyAnimation
{
    const Property* property = nullptr;
    std::vector<Object*> keyframes = {};
};

class ObjectType
{
public:
    ObjectType(TypeId id = TypeId::NoType) : id(id) {}

    TypeId id = TypeId::NoType;
    std::vector<const Property*> properties;
    std::vector<const ObjectDefinition*> definitions;
    std::unordered_map<Identifier, const Property*> property_from_id;
    std::unordered_map<QString, const Property*> property_from_name;

    const Property* property(const QString& name) const
    {
        auto it = property_from_name.find(name);
        if ( it == property_from_name.end() )
            return nullptr;
        return it->second;
    }

    const Property* property(Identifier id) const
    {
        auto it = property_from_id.find(id);
        if ( it == property_from_id.end() )
            return nullptr;
        return it->second;
    }
};

class Object
{
public:
    Object(const ObjectType* type = nullptr)
    : type_(type)
    {}

    const ObjectType& type() const
    {
        return *type_;
    }

    const std::unordered_map<const Property*, QVariant>& properties() const
    {
        return properties_;
    }

    template<class T>
    bool set(const QString& name, T value)
    {
        if ( auto prop = type_->property(name) )
        {
            properties_[prop].setValue(value);
            return true;
        }
        return false;
    }

    bool set(const QString& name, const QVariant& value)
    {
        if ( auto prop = type_->property(name) )
        {
            properties_[prop] = value;
            return true;
        }
        return false;
    }


    void set(const Property* prop, const QVariant& value)
    {
        properties_[prop] = value;
    }


    template<class T>
    T get(const QString& name, T value = {}) const
    {
        if ( auto prop = type_->property(name) )
        {
            auto it = properties_.find(prop);
            if ( it != properties_.end() )
                return it->second.value<T>();
        }
        return value;
    }

    QVariant get_variant(const QString& name)
    {

        if ( auto prop = type_->property(name) )
        {
            auto it = properties_.find(prop);
            if ( it != properties_.end() )
                return it->second;
        }
        return {};
    }

    bool has(const QString& name) const
    {
        if ( auto prop = type_->property(name) )
            return properties_.count(prop);
        return false;
    }

    std::vector<PropertyAnimation>& animations()
    {
        return animations_;
    }

    bool has_type(TypeId type) const
    {
        for ( const auto& def : type_->definitions )
            if ( def->type_id == type )
                return true;
        return false;
    }

    std::vector<Object*>& children()
    {
        return children_;
    }

    explicit operator bool() const
    {
        return type_;
    }

    const ObjectDefinition* definition() const
    {
        return type_->definitions[0];
    }

private:
    const ObjectType* type_;
    std::unordered_map<const Property*, QVariant> properties_;
    std::vector<PropertyAnimation> animations_;
    std::vector<Object*> children_;
};

class TypeSystem : public QObject
{
    Q_OBJECT

public:
    const ObjectDefinition* get_definition(TypeId type_id);

    const ObjectType* get_type(TypeId type_id);

    Object object(TypeId type_id)
    {
        return Object(get_type(type_id));
    }

    QString type_name(TypeId type_id);

signals:
    void type_not_found(int type_id);

private:
    bool gather_definitions(ObjectType& type, TypeId type_id);

    std::unordered_map<TypeId, ObjectType> types;
};


} // namespace glaxnimate::io::rive
