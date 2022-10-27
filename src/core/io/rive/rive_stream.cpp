#include "rive_stream.hpp"

static constexpr const bool is_big_endian = QSysInfo::ByteOrder == QSysInfo::BigEndian;

static void to_system_endian(QByteArray& data)
{
    if ( is_big_endian )
        std::reverse(data.begin(), data.end());
}

glaxnimate::io::rive::RiveStream::RiveStream(QIODevice* file):
    RiveStream(file->readAll())
{
}

glaxnimate::io::rive::RiveStream::RiveStream(QByteArray data):
    data(std::move(data)),
    data_start(this->data.data()),
    data_end(data_start + this->data.size())
{
}

void glaxnimate::io::rive::RiveStream::on_overflow()
{
    error = true;
}

bool glaxnimate::io::rive::RiveStream::eof() const
{
    return data_start >= data_end;
}

bool glaxnimate::io::rive::RiveStream::has_error() const
{
    return error;
}

QByteArray glaxnimate::io::rive::RiveStream::read(qint64 max_size)
{
    if ( data_start + max_size < data_end )
    {
        data_start += max_size;
        return QByteArray(data_start - max_size, max_size);
    }

    on_overflow();
    return {};
}

quint8 glaxnimate::io::rive::RiveStream::next()
{
    if ( data_start < data_end )
    {
        ++data_start;
        return data_start[-1];
    }

    on_overflow();
    return {};
}

glaxnimate::io::rive::Uint glaxnimate::io::rive::RiveStream::read_uint()
{
    auto data = read(4);
    if ( data.size() == sizeof(Uint) )
    {
        to_system_endian(data);
        return *reinterpret_cast<Uint*>(data.data());
    }
    else
    {
        return 0;
    }
}

glaxnimate::io::rive::Float glaxnimate::io::rive::RiveStream::read_float()
{
    auto data = read(4);
    if ( data.size() == sizeof(Float) )
    {
        to_system_endian(data);
        return *reinterpret_cast<Float*>(data.data());
    }
    else
    {
        on_overflow();
        return 0;
    }
}

glaxnimate::io::rive::VarUint glaxnimate::io::rive::RiveStream::read_varuint()
{
    VarUint result = 0;
    VarUint shift = 0;
    while (true)
    {
        quint8 byte = next();
        if ( error )
            return 0;

        result |= VarUint(byte & 0x7f) << shift;

        if ( !(byte & 0x80) )
            return result;

        shift += 7;
    }
}

glaxnimate::io::rive::RawString glaxnimate::io::rive::RiveStream::read_raw_string()
{
    auto size = read_varuint();
    if ( error )
        return {};

    return read(size);
}

glaxnimate::io::rive::String glaxnimate::io::rive::RiveStream::read_string()
{
    return QString::fromUtf8(read_raw_string());
}

glaxnimate::io::rive::PropertyTable glaxnimate::io::rive::RiveStream::read_property_table()
{
    std::vector<VarUint> props;
    while ( true )
    {
        VarUint id = read_varuint();
        if ( error )
            return {};

        if ( id == 0 )
            break;

        props.push_back(id);
    }

    Uint current_int = 0;
    Uint bit = 8;

    glaxnimate::io::rive::PropertyTable table;

    for ( auto id : props )
    {
        if ( bit == 8 )
        {
            current_int = read_uint();
            if ( error )
                return {};
            bit = 0;
        }

        int type = (current_int >> bit) & 3;
        if ( type == 0 )
            table[id] = PropertyType::VarUint;
        else if ( type == 1 )
            table[id] = PropertyType::String;
        else if ( type == 2 )
            table[id] = PropertyType::Float;
        else if ( type == 3 )
            table[id] = PropertyType::Color;
    }

    return table;
}

void glaxnimate::io::rive::RiveStream::skip_value(glaxnimate::io::rive::PropertyType type)
{
    switch ( type )
    {
        case PropertyType::Bool:
        case PropertyType::VarUint:
            read_varuint();
            break;
        case PropertyType::Bytes:
        case PropertyType::String:
            read_string();
            break;
        case PropertyType::Float:
            read_float();
            break;
        case PropertyType::Color:
            read_uint();
            break;
    }
}
