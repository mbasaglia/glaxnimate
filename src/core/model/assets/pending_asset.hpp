/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QUrl>
#include <QByteArray>


namespace glaxnimate::model {

struct PendingAsset
{
    int id = 0;
    QUrl url;
    QByteArray data;
    QString name_alias;
    bool loaded = false;
};

} // namespace glaxnimate::model
