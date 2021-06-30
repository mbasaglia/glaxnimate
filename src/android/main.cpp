#include <QApplication>

#include <QMainWindow>
#include <QSplashScreen>
#include <QLabel>

#include "main_window.hpp"
#include "glaxnimate_app_android.hpp"

int main(int argc, char *argv[])
{
    GlaxnimateApp app(argc, argv);

    QSplashScreen sc;
    sc.setPixmap(QPixmap(":glaxnimate/splash.svg"));
    sc.show();
    app.processEvents();

    MainWindow window;
    sc.finish(&window);
    window.show();

    int ret = app.exec();
    return ret;
}
