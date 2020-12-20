#include "glaxnimate_app.hpp"

#include <QDir>
#include <QPalette>

#include "app/settings/settings.hpp"
#include "settings/plugin_settings_group.hpp"
#include "app/settings/palette_settings.hpp"
#include "app/settings/keyboard_shortcuts.hpp"
#include "app_info.hpp"
#include "settings/clipboard_settings.hpp"

static QVariantMap avail_icon_themes()
{
    QVariantMap avail_icon_themes;
    avail_icon_themes[app::settings::Settings::tr("Glaxnimate Default")] = "";
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

static QString default_icon_theme()
{
    QPalette palette = QGuiApplication::palette();
    if ( palette.color(QPalette::Button).value() < 100 )
        return "icons-dark";
    else
        return "icons";
}

static void set_icon_theme(const QVariant& v)
{
    QString theme_name = v.toString();

    if ( theme_name.isEmpty() )
        theme_name = default_icon_theme();

    QIcon::setThemeName(theme_name);
}

static QVariantMap avail_languages()
{
    QVariantMap avail_languages;
    for ( const QString& name : app::TranslationService::instance().available_languages().keys() )
    {
        avail_languages[name] = app::TranslationService::instance().available_languages()[name];
    }

    return avail_languages;
}

static void set_language(const QVariant& v)
{
    QString code = v.toString();
    app::TranslationService::instance().change_lang_code(code);
}


static void icon_theme_fixup()
{
    QString default_theme = default_icon_theme();
    QIcon::setFallbackThemeName(default_theme);

    QString old = QIcon::themeName();
    if ( old == "icons" || old == "icons-dark" )
        QIcon::setThemeName(default_theme);
    else
        set_icon_theme(old);
}


void GlaxnimateApp::load_settings_metadata() const
{
    using namespace app::settings;
    QString curr_lang = app::TranslationService::instance().current_language_code();

    Settings::instance().add_group(SettingGroup{"ui", tr("User Interface"), "preferences-desktop-theme", {
        //      slug            Label              Tooltip                    Type                default     choices             side effects
        Setting("language",     tr("Language"),    tr("Interface Language"),  Setting::String,    curr_lang,  avail_languages(),  set_language),
        Setting("icon_theme",   tr("Icon Theme"),  "",                        Setting::String,    "",         avail_icon_themes(), ::set_icon_theme),
        Setting("window_state", {},                {},                        Setting::Internal,  QByteArray{}),
        Setting("window_geometry", {},             {},                        Setting::Internal,  QByteArray{}),
        Setting("timeline_splitter", {},           {},                        Setting::Internal,  QByteArray{}),
    }});
    Settings::instance().add_group(SettingGroup{"defaults", tr("New Animation Defaults"), "video-webm", {
        //      slug            Label           Tooltip                  default min max
        Setting("width",        tr("Width"),    "",                         512, 0, 1000000),
        Setting("height",       tr("Height"),   "",                         512, 0, 1000000),
        Setting("fps",          tr("FPS"),      tr("Frames per second"),     60, 0, 1000),
        Setting("duration",     tr("Duration"), tr("Duration in seconds"),    3, 0, 90000),
    }});
    Settings::instance().add_group(SettingGroup{"open_save", tr("Open / Save"), "kfloppy", {
        Setting("max_recent_files", tr("Max Recent Files"), {},                                                 5, 0, 16),
        Setting("path",             {},                     {},                                                 Setting::Internal,  QString{}),
        Setting("recent_files",     {},                     {},                                                 Setting::Internal,  QStringList{}),
        Setting("backup_frequency", tr("Backup Frequency"), tr("How often to save a backup copy (in minutes)"), 5, 0, 60),
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
        Setting("edit_mask",        {},         {},                        Setting::Internal,  false),
        Setting("color_main",       {},         {},                        Setting::Internal,  "#ffffff"),
        Setting("color_secondary",  {},         {},                        Setting::Internal,  "#000000"),
        Setting("stroke_width",     {},         {},                        Setting::Internal,  1.),
        Setting("stroke_cap",       {},         {},                        Setting::Internal,  int(Qt::RoundCap)),
        Setting("stroke_join",      {},         {},                        Setting::Internal,  int(Qt::RoundJoin)),
        Setting("stroke_miter",     {},         {},                        Setting::Internal,  4.),
        Setting("star_type",        {},         {},                        Setting::Internal,  1),
        Setting("star_ratio",       {},         {},                        Setting::Internal,  0.5),
        Setting("star_points",      {},         {},                        Setting::Internal,  5),
    }});
}


static void load_themes(GlaxnimateApp* app, app::settings::PaletteSettings* settings)
{
    for ( QDir themedir : app->data_paths("themes") )
    {
        for ( const auto& theme : themedir.entryList({"*.ini"}, QDir::Files|QDir::Readable, QDir::Name|QDir::IgnoreCase) )
        {
            QSettings ini_parser(themedir.absoluteFilePath(theme), QSettings::IniFormat);
            settings->load_palette(ini_parser);
        }
    }
}


void GlaxnimateApp::on_initialize()
{
    setWindowIcon(QIcon(data_file("images/logo.svg")));

    QStringList search_paths = data_paths("icons");
    search_paths += QIcon::themeSearchPaths();
    QIcon::setThemeSearchPaths(search_paths);

    app::settings::Settings::instance().add_custom_group(std::make_unique<settings::PluginSettingsGroup>(QStringList{
        "AnimatedRaster", "ReplaceColor", "dotLottie"
    }));
    app::settings::Settings::instance().add_custom_group(std::make_unique<settings::ClipboardSettings>());

    connect(this, &QGuiApplication::paletteChanged, this, &icon_theme_fixup);
    auto palette_settings = std::make_unique<app::settings::PaletteSettings>();
    load_themes(this, palette_settings.get());
    app::settings::Settings::instance().add_custom_group(std::move(palette_settings));

    auto sc_settings = std::make_unique<app::settings::ShortcutSettings>();
    shortcut_settings = sc_settings.get();
    app::settings::Settings::instance().add_custom_group(std::move(sc_settings));

    QDir().mkpath(backup_path());

    app::log::Logger::instance().add_listener<app::log::ListenerStderr>();
    store_logger = app::log::Logger::instance().add_listener<app::log::ListenerStore>();
}

QString GlaxnimateApp::backup_path(const QString& file) const
{
    return writable_data_path("backup/"+file);
}

app::settings::ShortcutSettings * GlaxnimateApp::shortcuts() const
{
    return shortcut_settings;
}
