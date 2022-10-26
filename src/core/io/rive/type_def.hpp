#pragma once

#include <vector>
#include <unordered_map>
#include <QString>
#include <QVariant>

namespace glaxnimate::io::rive {

using Identifier = quint64;

enum class PropertyType
{
    VarUint,// VarUint
    Bool,   // Byte
    String, // String
    Bytes,  // Raw String
    Float,  // Float
    Color,  // Uint
};

struct Property
{
    QString name;
    PropertyType type = PropertyType::VarUint;
};

struct ObjectDefinition
{
    QString name;
    Identifier extends = 0;
    std::unordered_map<Identifier, Property> properties;
};

struct Object
{
    Identifier definition_id = 0;
    std::vector<const ObjectDefinition*> definitions;
    QVariantMap properties;
    std::unordered_map<Identifier, Property> property_definitions;
};

extern std::unordered_map<Identifier, ObjectDefinition> defined_objects;


} // namespace glaxnimate::io::rive
