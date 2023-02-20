/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDir>
#include <QVariant>

namespace glaxnimate::io {


class ImportExport;

struct Options
{
    ImportExport* format = nullptr;
    QDir path;
    QString filename;
    QVariantMap settings;

    void clear()
    {
        *this = {};
    }
};


} // namespace glaxnimate::io
