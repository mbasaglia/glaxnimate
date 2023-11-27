/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QtGlobal>
#include <QString>

// Qt provides some literals but they change based on Qt version
inline QString operator""_qs(const char* str, std::size_t size)
{
    return QString::fromUtf8(str, size);
}

inline QChar operator""_qc(char c)
{
    return QChar(int(c));
}
