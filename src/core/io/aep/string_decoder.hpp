/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QString>
#include <QByteArray>

namespace glaxnimate::io::aep {
    QString decode_string(const QByteArray& data);

    QString decode_utf16(const QByteArray& data, bool big_endian);
} // namespace glaxnimate::io::aep
