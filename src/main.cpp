#include <QApplication>
#include <QMainWindow>
#include "application_info_generated.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QMainWindow window;
    window.setWindowTitle(QString("%1 %2").arg(PROJECT_NAME).arg(PROJECT_VERSION));
    window.show();
    return app.exec();
}
