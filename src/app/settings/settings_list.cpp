#include "settings.hpp"
#include "app/settings/settings.hpp"
#include "app/app_info.hpp"
#include "app/translation_service.hpp"

#include <QDir>
#include <QGuiApplication>
#include <QPalette>

namespace {

QVariantMap avail_icon_themes()
{
    QVariantMap avail_icon_themes;
    avail_icon_themes[app::settings::Settings::tr("Default")] = "";
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

QVariantMap avail_languages()
{
    QVariantMap avail_languages;
    for ( const QString& name : app::TranslationService::instance().available_languages().keys() )
    {
        avail_languages[name] = app::TranslationService::instance().available_languages()[name];
    }

    return avail_languages;
}

void set_language(const QVariant& v)
{
    QString code = v.toString();
    app::TranslationService::instance().change_lang_code(code);
}

} // namespace

/// @todo move data loading (and QSettings creation) somewhere in AppInfo, so the settings system is more reusable
void app::settings::Settings::load_metadata()
{
    QString curr_lang = app::TranslationService::instance().current_language_code();

    Settings::instance().add_group(SettingGroup{"ui", tr("User Interface"), "preferences-desktop-theme", {
        //      slug            Label              Tooltip                    Type                default     choices             side effects
        Setting("language",     tr("Language"),    tr("Interface Language"),  Setting::String,    curr_lang,  avail_languages(),  set_language),
        Setting("icon_theme",   tr("Icon Theme"),  "",                        Setting::String,    "",         avail_icon_themes(), set_icon_theme),
        Setting("window_state", {},                {},                        Setting::Internal,  QByteArray{}),
        Setting("window_geometry", {},             {},                        Setting::Internal,  QByteArray{}),
    }});
    Settings::instance().add_group(SettingGroup{"defaults", tr("New Animation Defaults"), "document-new", {
        //      slug            Label           Tooltip                  default min max
        Setting("width",        tr("Width"),    "",                         512, 0, 1000000),
        Setting("height",       tr("Height"),   "",                         512, 0, 1000000),
        Setting("frame_rate",   tr("FPS"),      tr("Frames per second"),     60, 0, 1000),
        Setting("duration",     tr("Duration"), tr("Duration in seconds"),    3, 0, 90000),
    }});
}
