/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "gzip.hpp"

#include <array>
#include <cstring>

#include <QFile>
#include <QApplication>
#include <QBuffer>
#include <KCompressionDevice>


using namespace glaxnimate;


bool utils::gzip::decompress(QIODevice& input, QByteArray& output, const utils::gzip::ErrorFunc& on_error)
{
    KCompressionDevice compressed(&input, false, KCompressionDevice::GZip);
    compressed.open(QIODevice::ReadOnly);
    output = compressed.readAll();

    if ( compressed.error() )
    {
        on_error(QApplication::tr("Could not decompress data"));
        return false;
    }

    return true;
}


bool utils::gzip::decompress(const QByteArray& input, QByteArray& output, const utils::gzip::ErrorFunc& on_error)
{
    QBuffer buf(const_cast<QByteArray*>(&input));
    return decompress(buf, output, on_error);
}

bool utils::gzip::is_compressed(QIODevice& input)
{
    return input.peek(2) == "\x1f\x8b";
}

bool utils::gzip::is_compressed(const QByteArray& input)
{
    return input.size() >= 2 && input[0] == '\x1f' && input[1] == '\x8b';
}

