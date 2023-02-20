/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDesktopServices>
#include <QUrl>

namespace app::desktop {

inline bool open_file(const QString& path)
{
    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

} // namespace app::desktop
