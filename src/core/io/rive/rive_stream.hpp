#pragma once

#include <unordered_map>
#include <cstdint>

#include <QIODevice>
#include <QColor>
#include <QVariant>

namespace glaxnimate::io::rive {

enum class UnknownPropertyType
{
    Uint = 0,
    String = 1,
    Float = 2,
    Color = 3
};

using VarUint = std::uint64_t;
using Uint = std::uint32_t;
using Float = float;
using RawString = QByteArray;
using String = QString;
using PropertyTable = std::unordered_map<VarUint, UnknownPropertyType>;

class RiveStream
{
public:

    explicit RiveStream(QIODevice* file);

    Uint read_uint();
    Float read_float();
    VarUint read_varuint();
    RawString read_raw_string();
    String read_string();
    void skip_value(UnknownPropertyType type);

    bool eof() const;
    bool has_error() const;

    QByteArray read(qint64 max_size);
    quint8 next();

    PropertyTable read_property_table();

private:
    void on_overflow();

private:
    QByteArray data;
    const char* data_start;
    const char* data_end;
    bool error = false;
};

} // namespace glaxnimate::io::rive
