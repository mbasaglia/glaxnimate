#include "app/settings/settings.hpp"
#include "app/app_info.hpp"

#include <QDir>
#include <QGuiApplication>
#include <QPalette>

namespace {

QVariantMap avail_icon_themes()
{
    QVariantMap avail_icon_themes;
    avail_icon_themes[QObject::tr("Default")] = "";
    for ( QDir search : QIcon::themeSearchPaths() )
    {
        for ( const auto& avail : search.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot) )
        {
            QDir subdir(avail.filePath());
            if ( subdir.exists("index.theme") )
                avail_icon_themes[avail.baseName()] = avail.baseName();
        }
    }

    return avail_icon_themes;
}

void set_icon_theme(const QVariant& v)
{
    QString theme_name = v.toString();

    if ( theme_name.isEmpty() )
    {
        QPalette palette = QGuiApplication::palette();
        if ( palette.color(QPalette::Button).value() < 100 )
            theme_name = "icons-dark";
        else
            theme_name = "icons";
    }

    QIcon::setThemeName(theme_name);
}

} // namespace
/// @todo move data loading (and QSettings creation) somewhere in AppInfo, so the settings system is more reusable
void app::settings::Settings::load_metadata()
{
    add_group(SettingGroup{"ui", QObject::tr("User Interface"), "preferences-desktop-theme", {
        //      slug            Label                       Tooltip                             Type                default     choices             side effects
        Setting("language",     QObject::tr("Language"),    QObject::tr("Interface Language"),  Setting::String,    "en"),
        Setting("icon_theme",   QObject::tr("Icon Theme"),  "",                                 Setting::String,    "",         avail_icon_themes(), set_icon_theme),
        Setting("window_state", {},                         {},                                 Setting::Internal,  QByteArray{}),
        Setting("window_geometry", {},                      {},                                 Setting::Internal,  QByteArray{}),
    }});
    add_group(SettingGroup{"defaults", QObject::tr("New Animation Defaults"), "document-new", {
        //      slug            Label                   Tooltip                         default min max
        Setting("width",        QObject::tr("Width"),   "",                                 512, 0, 1000000),
        Setting("height",       QObject::tr("Height"),  "",                                 512, 0, 1000000),
        Setting("frame_rate",   QObject::tr("FPS"),     QObject::tr("Frames per second"),    60, 0, 1000),
        Setting("duration",     QObject::tr("Duration"),QObject::tr("Duration in seconds"),   3, 0, 90000),
    }});
}
