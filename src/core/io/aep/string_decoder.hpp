/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#   include <QStringConverter>
namespace glaxnimate::io::aep {
    QString decode_string(const QByteArray& data)
    {
        auto encoding = QStringConverter::encodingForData(data);
        return QStringDecoder(encoding).decode(data);
    }

    QString decode_utf16(const QByteArray& data, bool big_endian)
    {
        auto encoding = big_endian ? QStringConverter::Utf16BE : QStringConverter::Utf16LE;
        return QStringDecoder(encoding).decode(data);
    }
} // namespace glaxnimate::io::aep
#else
#   include <QTextCodec>
namespace glaxnimate::io::aep {
    QString decode_string(const QByteArray& data)
    {
        auto utf8 = QTextCodec::codecForName("UTF-8");
        auto encoding = QTextCodec::codecForUtfText(data, utf8);
        return encoding->toUnicode(data);
    }

    QString decode_utf16(const QByteArray& data, bool big_endian)
    {
        const char* name = big_endian ? "UFT-16BE" : "UFT-16BE";
        auto encoding = QTextCodec::codecForName(name);
        return encoding->toUnicode(data);
    }
} // namespace glaxnimate::io::aep
#endif
