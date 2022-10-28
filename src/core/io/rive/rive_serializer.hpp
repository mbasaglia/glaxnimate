#pragma once

#include "io/binary_stream.hpp"

#include "type_def.hpp"

namespace glaxnimate::io::rive {

struct OutputObject : public ObjectType
{
    TypeId type_id = TypeId::NoType;
    std::unordered_map<Identifier, std::pair<PropertyType, QVariant>> properties;

    template<class T>
    bool set(const QString& name, T value)
    {
        auto it_id = property_names.find(name);
        if ( it_id == property_names.end() )
            return false;

        auto it_def = property_definitions.find(it_id->second);
        if ( it_def == property_definitions.end() )
            return false;

        properties[it_id->second] = {it_def->second.type, QVariant::fromValue(value)};
        return true;
    }
};


class RiveSerializer
{
public:
    void write_header(int vmaj, int vmin, Identifier file_id);

    void write_property_table(const PropertyTable& properties);

    void write_object(const OutputObject& output);

private:
    BinaryOutputStream stream;
};

} // namespace glaxnimate::io::rive
