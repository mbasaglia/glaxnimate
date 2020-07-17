#include <QApplication>

#include "ui/dialogs/glaxnimate_window.hpp"
#include "app/app_info.hpp"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    AppInfo& info = AppInfo::instance();
    app.setApplicationName(info.slug());
    app.setApplicationDisplayName(info.name());
    app.setApplicationVersion(info.version());
    app.setOrganizationName(info.organization());

    QStringList search_paths = info.data_paths("img/icons");
    search_paths += QIcon::themeSearchPaths();
    QIcon::setThemeSearchPaths(search_paths);
    QIcon::setThemeName("breeze");

    GlaxnimateWindow window;
    window.show();
    return app.exec();
}
