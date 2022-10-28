#include "rive_serializer.hpp"
#include <QColor>

void glaxnimate::io::rive::RiveSerializer::write_header(int vmaj, int vmin, glaxnimate::io::rive::Identifier file_id)
{
    stream.write("RIVE");
    stream.write_uint_leb128(vmaj);
    stream.write_uint_leb128(vmin);
    stream.write_uint_leb128(file_id);
}

void glaxnimate::io::rive::RiveSerializer::write_property_table(const glaxnimate::io::rive::PropertyTable& properties)
{
    for ( const auto& p: properties )
        stream.write_uint_leb128(p.first);

    stream.write_byte(0);

    quint32 current_int = 0;
    quint32 bit = 0;
    for ( const auto& p: properties )
    {
        int type = 0;
        switch ( p.second )
        {
            case PropertyType::VarUint:
            case PropertyType::Bool:
                type = 0;
                break;
            case PropertyType::Bytes:
            case PropertyType::String:
                type = 1;
                break;
            case PropertyType::Float:
                type = 2;
                break;
            case PropertyType::Color:
                type = 3;
                break;
        }

        current_int << 2;
        current_int |= type;
        bit += 2;
        if ( bit == 8 )
        {
            stream.write_uint32_le(current_int);
            bit = 0;
            current_int = 0;
        }
    }
    if ( bit != 0 )
        stream.write_uint32_le(current_int);

    Q_UNUSED(properties);
}

void glaxnimate::io::rive::RiveSerializer::write_object(const glaxnimate::io::rive::OutputObject& output)
{
    stream.write_uint_leb128(VarUint(output.type_id));
    for ( const auto& p : output.properties )
    {
        if ( !p.second.second.isValid() )
            continue;
        stream.write_uint_leb128(p.first);
        write_property_value(p.second.first, p.second.second);
    }
}

void glaxnimate::io::rive::RiveSerializer::write_property_value(glaxnimate::io::rive::PropertyType id, const QVariant& value)
{
    switch ( id )
    {
        case PropertyType::Bool:
            stream.write_byte(value.toBool());
            return;
        case PropertyType::VarUint:
            stream.write_uint_leb128(value.value<VarUint>());
            return;
        case PropertyType::Color:
            stream.write_uint32_le(value.value<QColor>().rgba());
            return;
        case PropertyType::Bytes:
        {
            auto data = value.toByteArray();
            stream.write_uint32_le(data.size());
            stream.write(data);
            return;
        }
        case PropertyType::String:
        {
            auto data = value.toString().toUtf8();
            stream.write_uint32_le(data.size());
            stream.write(data);
            return;
        }
        case PropertyType::Float:
            stream.write_float32_le(value.toFloat());
    }
}
