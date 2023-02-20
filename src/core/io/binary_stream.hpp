/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <unordered_map>
#include <QIODevice>


#include "io/binary_types.hpp"

namespace glaxnimate::io {


class BinaryInputStream
{
public:
    explicit BinaryInputStream(QIODevice* file);
    explicit BinaryInputStream(QByteArray data);

    quint32 read_uint32_le();
    Float32 read_float32_le();
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

class BinaryOutputStream
{
public:
    explicit BinaryOutputStream(QIODevice* file);

    void write_uint32_le(quint32 v);
    void write_float32_le(Float32 v);
    void write_uint_leb128(VarUint v);
    void write_byte(quint8 v);
    void write(const QByteArray& data);

private:
    QIODevice* file = nullptr;
};

} // namespace glaxnimate::io
