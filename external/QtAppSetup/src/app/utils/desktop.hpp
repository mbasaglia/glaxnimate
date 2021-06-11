#pragma once

#include <QDesktopServices>
#include <QUrl>

namespace app::desktop {

inline bool open_file(const QString& path)
{
    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

} // namespace app::desktop
