#include <QApplication>

#include "ui/dialogs/glaxnimate_window.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    GlaxnimateWindow window;
    window.show();
    return app.exec();
}
