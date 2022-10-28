#include "binary_stream.hpp"

#include <QtEndian>

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
    static_assert(sizeof(quint32) == 4);
    auto data = read(4);
    if ( data.size() == 4 )
    {
        return qFromLittleEndian<quint32>(data.data());
    }
    else
    {
        return 0;
    }
}

glaxnimate::io::Float32 glaxnimate::io::BinaryInputStream::read_float32_le()
{
    static_assert(sizeof(Float32) == 4);
    auto data = read(4);
    if ( data.size() == 4 )
    {
        return qFromLittleEndian<Float32>(data.data());
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

glaxnimate::io::BinaryOutputStream::BinaryOutputStream(QIODevice* file)
    : file(file)
{
}

void glaxnimate::io::BinaryOutputStream::write(const QByteArray& data)
{
    file->write(data);
}

void glaxnimate::io::BinaryOutputStream::write_byte(quint8 v)
{
    file->putChar(v);
}

void glaxnimate::io::BinaryOutputStream::write_float32_le(glaxnimate::io::Float32 v)
{
    static_assert(sizeof(Float32) == 4);
    std::array<quint8, 4> data;
    qToLittleEndian(v, data.data());
    file->write((const char*)data.data(), 4);
}

void glaxnimate::io::BinaryOutputStream::write_uint32_le(quint32 v)
{
    static_assert(sizeof(quint32) == 4);
    std::array<quint8, 4> data;
    qToLittleEndian(v, data.data());
    file->write((const char*)data.data(), 4);
}

void glaxnimate::io::BinaryOutputStream::write_uint_leb128(glaxnimate::io::VarUint v)
{
    while ( true )
    {
        quint8 byte = v & 0x7f;
        v >>= 7;
        if ( v == 0 )
        {
            write_byte(byte);
            break;
        }
        write_byte(byte | 0x80);
    }
}
