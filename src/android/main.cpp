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
    using namespace glaxnimate::android;

    GlaxnimateApp app(argc, argv);

    AppInfo::instance().init_qapplication();
    app.setStyle(new AndroidStyle);
    app.setStyleSheet(R"(
QPushButton {
    border: 1px solid #8f8f8f;
    border-radius: 6px;
    background-color: #f3f3f3;
}

QToolButton {
    border: 1px solid transparent;
    border-radius: 6px;
    background-color: transparent;
}

QToolButton:pressed, QToolButton:checked, QPushButton:pressed, QPushButton:checked {
    border: 1px solid #8f8f8f;
    background-color: #dedede;
}
)");

    MainWindow window;
    window.show();

    int ret = app.exec();
    return ret;
}
