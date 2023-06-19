/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <QtGlobal>

#include "io/aep/string_decoder.hpp"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

#include <QStringConverter>
QString glaxnimate::io::aep::decode_string(const QByteArray& data)
{
    auto encoding = QStringConverter::encodingForData(data);
    if ( encoding )
        return QStringDecoder(encoding).decode(data);
    return QStringDecoder(QStringConverter::Utf8).decode(data);
}

QString glaxnimate::io::aep::decode_utf16(const QByteArray& data, bool big_endian)
{
    auto encoding = big_endian ? QStringConverter::Utf16BE : QStringConverter::Utf16LE;
    return QStringDecoder(encoding).decode(data);
}

#else

#include <QTextCodec>
QString glaxnimate::io::aep::decode_string(const QByteArray& data)
{
    auto utf8 = QTextCodec::codecForName("UTF-8");
    auto encoding = QTextCodec::codecForUtfText(data, utf8);
    return encoding->toUnicode(data);
}

QString glaxnimate::io::aep::decode_utf16(const QByteArray& data, bool big_endian)
{
    const char* name = big_endian ? "UFT-16BE" : "UFT-16BE";
    auto encoding = QTextCodec::codecForName(name);
    return encoding->toUnicode(data);
}

#endif

