#include <QApplication>

#include <QMainWindow>
#include <QSplashScreen>
#include <QLabel>

#include "main_window.hpp"
#include "glaxnimate_app_android.hpp"
#include "app_info.hpp"

int main(int argc, char *argv[])
{
    GlaxnimateApp app(argc, argv);

    QIcon::setFallbackSearchPaths({":glaxnimate/images/icons/"});
    AppInfo::instance().init_qapplication();

    MainWindow window;
    window.show();

    int ret = app.exec();
    return ret;
}
