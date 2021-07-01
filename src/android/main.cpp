#include <QApplication>

#include <QMainWindow>
#include <QSplashScreen>
#include <QLabel>

#include "main_window.hpp"
#include "glaxnimate_app_android.hpp"
#include "app_info.hpp"
#include "android_style.hpp"

int main(int argc, char *argv[])
{
    GlaxnimateApp app(argc, argv);

    AppInfo::instance().init_qapplication();
    app.setStyle(new AndroidStyle);

    MainWindow window;
    window.show();

    int ret = app.exec();
    return ret;
}
