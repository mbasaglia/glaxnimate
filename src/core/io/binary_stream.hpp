#pragma once

#include <unordered_map>
#include <cstdint>

#include <QIODevice>

namespace glaxnimate::io {

using VarUint = std::uint64_t;
using Float32 = float;

class BinaryInputStream
{
public:

    explicit BinaryInputStream(QIODevice* file);
    explicit BinaryInputStream(QByteArray data);

    quint32 read_uint32_le();
    Float32 read_float32();
    VarUint read_uint_leb128();

    bool eof() const;
    bool has_error() const;

    QByteArray read(qint64 max_size);
    quint8 next();

private:
    void on_overflow();

private:
    QByteArray data;
    const char* data_start;
    const char* data_end;
    bool error = false;
};

} // namespace glaxnimate::io
