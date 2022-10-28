#pragma once
#include "type_def.hpp"

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
    const ObjectType* get_type(TypeId type_id, bool null_on_error)
    {
        auto it = types.find(type_id);
        if ( it != types.end() )
            return &it->second;

        ObjectType type(type_id);
        if ( !gather_definitions(type, type_id) && null_on_error )
            return nullptr;

        return &types.emplace(type_id, std::move(type)).first->second;
    }

    Object object(TypeId type_id)
    {
        return Object(get_type(type_id, true));
    }

signals:
    void type_not_found(int type_id);

private:
    bool gather_definitions(ObjectType& type, TypeId type_id)
    {
        auto it = defined_objects.find(type_id);
        if ( it == defined_objects.end() )
        {
            emit type_not_found(int(type_id));
            return false;
        }

        const auto& def = it->second;

        type.definitions.push_back(&def);

        if ( def.extends != TypeId::NoType )
        {
            if ( !gather_definitions(type, def.extends) )
                return false;
        }

        for ( const auto& prop : def.properties )
        {
            type.property_from_name[prop.name] = &prop;
            type.property_from_id[prop.id] = &prop;
            type.properties.push_back(&prop);
        }

        return true;
    }

    std::unordered_map<TypeId, ObjectType> types;
};


} // namespace glaxnimate::io::rive
