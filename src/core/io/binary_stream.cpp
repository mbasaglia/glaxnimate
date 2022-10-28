#include "binary_stream.hpp"

static constexpr const bool is_big_endian = QSysInfo::ByteOrder == QSysInfo::BigEndian;

static void to_system_endian(QByteArray& data)
{
    if ( is_big_endian )
        std::reverse(data.begin(), data.end());
}

glaxnimate::io::BinaryInputStream::BinaryInputStream(QIODevice* file):
    BinaryInputStream(file->readAll())
{
}

glaxnimate::io::BinaryInputStream::BinaryInputStream(QByteArray data):
    data(std::move(data)),
    data_start(this->data.data()),
    data_end(data_start + this->data.size())
{
}

void glaxnimate::io::BinaryInputStream::on_overflow()
{
    error = true;
}

bool glaxnimate::io::BinaryInputStream::eof() const
{
    return data_start >= data_end;
}

bool glaxnimate::io::BinaryInputStream::has_error() const
{
    return error;
}

QByteArray glaxnimate::io::BinaryInputStream::read(qint64 max_size)
{
    if ( data_start + max_size < data_end )
    {
        data_start += max_size;
        return QByteArray(data_start - max_size, max_size);
    }

    on_overflow();
    return {};
}

quint8 glaxnimate::io::BinaryInputStream::next()
{
    if ( data_start < data_end )
    {
        ++data_start;
        return data_start[-1];
    }

    on_overflow();
    return {};
}

quint32 glaxnimate::io::BinaryInputStream::read_uint32_le()
{
    auto data = read(4);
    if ( data.size() == 4 )
    {
        auto bytes = reinterpret_cast<quint8*>(data.data());
        return (bytes[3] << 24ull) | (bytes[2] << 16ull) | (bytes[1] << 8ull) | bytes[0];
    }
    else
    {
        return 0;
    }
}

glaxnimate::io::Float32 glaxnimate::io::BinaryInputStream::read_float32()
{
    static_assert(sizeof(Float32) == 4);
    auto data = read(4);
    if ( data.size() == sizeof(Float32) )
    {
        to_system_endian(data);
        return *reinterpret_cast<Float32*>(data.data());
    }
    else
    {
        on_overflow();
        return 0;
    }
}

glaxnimate::io::VarUint glaxnimate::io::BinaryInputStream::read_uint_leb128()
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
