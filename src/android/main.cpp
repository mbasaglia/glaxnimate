#include <QApplication>

#include <QMainWindow>
#include <QSplashScreen>
#include <QLabel>

#include "main_window.hpp"
#include "glaxnimate_app.hpp"
#include "app_info.hpp"
#include "android_style.hpp"
#include "android_intent_handler.hpp"

#include "android_file_picker.hpp"
#include <QDebug>

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

QMenu {
    overflow: hidden;
    border: 1px solid #8f8f8f;
    margin: 0;
    padding: -1px;
    border-radius: 6px;
    background-color: #f3f3f3;
    color: #000;
}

QMenu::item {
    padding: 2px 25px 2px 20px;
    border: 1px solid transparent;
    min-width: 400px;
}

QMenu::item:selected, QMenu::item:checked {
    border-color: #8f8f8f;
    background: #dedede;
}
)");

    app.initialize();

#ifdef Q_OS_ANDROID_FAKE
    QDir d(__FILE__); d.cdUp(); d.cdUp(); d.cdUp();
    QIcon::setThemeSearchPaths({d.filePath("data/icons/breeze-icons/")});
#else
    QIcon::setThemeSearchPaths({"assets:/icons"});
#endif
    QIcon::setThemeName("icons");
//    QIcon::setFallbackSearchPaths({"assets:/icons/icons"});



    qDebug() << "\n\n\x1b[31m================================\x1b[m";
    qDebug() << AndroidFilePicker::list_assets("icons/icons");
    qDebug() << AndroidFilePicker::list_assets("images/icons");
    qDebug() << GlaxnimateApp::instance()->data_file("images/icons/keyframe-record.svg");
    QIcon icon("assets:/images/icons/keyframe-record.svg");
    qDebug() << icon.isNull() << icon.pixmap(24).isNull();
    qDebug() << "\x1b[31m================================\x1b[m\n\n";

    MainWindow window;
    window.show();

    QUrl intent = AndroidIntentHandler::instance()->view_uri();
    if ( !intent.isEmpty() )
        window.open_intent(intent);

    QObject::connect(
        AndroidIntentHandler::instance(), &AndroidIntentHandler::view_uri_changed,
        &window, &MainWindow::open_intent,
        Qt::QueuedConnection
    );

    int ret = app.exec();

    app.finalize();

    return ret;
}
