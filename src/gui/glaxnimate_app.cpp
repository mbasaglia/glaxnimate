#include "glaxnimate_app.hpp"

#include <QDir>
#include <QPalette>

#include "app/settings/settings.hpp"
#include "app_info.hpp"
#include "app/scripting/plugin_settings_group.hpp"

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


void GlaxnimateApp::load_settings_metadata() const
{
    using namespace app::settings;
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
        Setting("fps",          tr("FPS"),      tr("Frames per second"),     60, 0, 1000),
        Setting("duration",     tr("Duration"), tr("Duration in seconds"),    3, 0, 90000),
    }});
    Settings::instance().add_group(SettingGroup{"open_save", tr("Open / Save"), "document-save", {
        Setting("max_recent_files", tr("Max Recent Files"), {},               5, 0, 16),
        Setting("path",         {},             {},                        Setting::Internal,  QString{}),
        Setting("recent_files", {},             {},                        Setting::Internal,  QStringList{}),
    }});
    Settings::instance().add_group(SettingGroup{"scripting", tr("Scripting"), "utilities-terminal", {
        //      slug            Label           Tooltip                    Type                default
        Setting("history",      {},             {},                        Setting::Internal,  QStringList{}),
        Setting("max_history",  {},             {},                        Setting::Internal,  100),
    }});
    Settings::instance().add_group(SettingGroup{"tools", tr("Tools"), "tools", {
        //      slug                Label       Tooltip                    Type                default
        Setting("shape_group",      {},         {},                        Setting::Internal,  true),
        Setting("shape_fill",       {},         {},                        Setting::Internal,  true),
        Setting("shape_stroke",     {},         {},                        Setting::Internal,  true),
        Setting("shape_transform",  {},         {},                        Setting::Internal,  true),
        Setting("color_main",       {},         {},                        Setting::Internal,  "#ffffff"),
        Setting("color_secondary",  {},         {},                        Setting::Internal,  "#000000"),
        Setting("stroke_width",     {},         {},                        Setting::Internal,  1.),
        Setting("stroke_cap",       {},         {},                        Setting::Internal,  int(Qt::RoundCap)),
        Setting("stroke_join",      {},         {},                        Setting::Internal,  int(Qt::RoundJoin)),
        Setting("stroke_miter",     {},         {},                        Setting::Internal,  4.),
    }});
}

void GlaxnimateApp::on_initialize()
{
    AppInfo& info = AppInfo::instance();
    setApplicationName(info.slug());
    setApplicationDisplayName(info.name());
    setApplicationVersion(info.version());
    setOrganizationName(info.organization());
    setWindowIcon(QIcon(data_file("images/logo.svg")));

    QStringList search_paths = data_paths("icons");
    search_paths += QIcon::themeSearchPaths();
    QIcon::setThemeSearchPaths(search_paths);


    app::settings::Settings::instance().add_custom_group(std::make_unique<app::scripting::PluginSettingsGroup>(QStringList{
    }));

    app::log::Logger::instance().add_listener<app::log::ListenerStderr>();
    store_logger = app::log::Logger::instance().add_listener<app::log::ListenerStore>();
}
