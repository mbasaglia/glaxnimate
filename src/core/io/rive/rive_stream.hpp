#pragma once

#include <unordered_map>
#include <cstdint>

#include <QIODevice>
#include <QColor>
#include <QVariant>
#include "type_def.hpp"

namespace glaxnimate::io::rive {


using VarUint = std::uint64_t;
using Uint = std::uint32_t;
using Float = float;
using RawString = QByteArray;
using String = QString;
using PropertyTable = std::unordered_map<VarUint, PropertyType>;

class RiveStream
{
public:

    explicit RiveStream(QIODevice* file);
    explicit RiveStream(QByteArray data);

    Uint read_uint();
    Float read_float();
    VarUint read_varuint();
    RawString read_raw_string();
    String read_string();
    void skip_value(PropertyType type);

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
