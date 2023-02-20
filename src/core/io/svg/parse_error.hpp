/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QString>
#include <exception>

namespace glaxnimate::io::svg {


class SvgParseError : public std::exception
{
public:
    QString formatted(const QString& filename) const
    {
        return QString("%1:%2:%3: XML Parse Error: %4")
            .arg(filename)
            .arg(line)
            .arg(column)
            .arg(message)
        ;
    }

    QString message;
    int line = -1;
    int column = -1;
};

} // namespace glaxnimate::io::svg
