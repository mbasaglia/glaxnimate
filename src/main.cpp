#include <QApplication>

#include "ui/dialogs/glaxnimate_window.hpp"
#include "app/app_info.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(AppInfo::instance().slug());
    app.setApplicationDisplayName(AppInfo::instance().name());
    app.setApplicationVersion(AppInfo::instance().version());
    app.setOrganizationName(AppInfo::instance().organization());

    GlaxnimateWindow window;
    window.show();
    return app.exec();
}
