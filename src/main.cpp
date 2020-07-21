#include <QApplication>

#include "ui/dialogs/glaxnimate_window.hpp"
#include "app/app_info.hpp"
#include "app/settings/settings.hpp"
#include "app/translation_service.hpp"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    AppInfo& info = AppInfo::instance();
    app.setApplicationName(info.slug());
    app.setApplicationDisplayName(info.name());
    app.setApplicationVersion(info.version());
    app.setOrganizationName(info.organization());
    app.setWindowIcon(QIcon(info.data_file("icon.svg")));

    QStringList search_paths = info.data_paths("icons");
    search_paths += QIcon::themeSearchPaths();
    QIcon::setThemeSearchPaths(search_paths);


    app::TranslationService::instance().initialize();
    app::settings::Settings::instance().load();

    GlaxnimateWindow window;
    window.show();
    int ret = app.exec();

    app::settings::Settings::instance().save();

    return ret;
}
