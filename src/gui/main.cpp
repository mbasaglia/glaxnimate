#include <QSplashScreen>

#include "glaxnimate_app.hpp"
#include "widgets/dialogs/glaxnimate_window.hpp"

int main(int argc, char *argv[])
{
    GlaxnimateApp app(argc, argv);

    QSplashScreen sc;
    sc.setPixmap(QPixmap(":glaxnimate/splash.svg"));
    sc.show();
    app.processEvents();

    app.initialize();

    GlaxnimateWindow window;
    sc.finish(&window);
    window.show();

    int ret = app.exec();

    app.finalize();

    return ret;
}
