#include <QApplication>
#include <QMainWindow>
#include "application_info_generated.h"

#include "model/document.hpp"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QMainWindow window;
    window.setWindowTitle(QString("%1 %2").arg(PROJECT_NAME).arg(PROJECT_VERSION));
    window.show();
    return app.exec();
}
