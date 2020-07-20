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
    app.setWindowIcon(QIcon(info.data_file("icon.svg")));

    QStringList search_paths = info.data_paths("icons");
    search_paths += QIcon::themeSearchPaths();
    QIcon::setThemeSearchPaths(search_paths);

    QPalette palette = QGuiApplication::palette();
    if ( palette.color(QPalette::Button).value() < 100 )
        QIcon::setThemeName("icons-dark");
    else
        QIcon::setThemeName("icons");

    GlaxnimateWindow window;
    window.show();
    return app.exec();
}
