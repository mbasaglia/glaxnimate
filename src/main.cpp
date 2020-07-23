#include "glaxnimate_app.hpp"
#include "ui/dialogs/glaxnimate_window.hpp"


int main(int argc, char *argv[])
{
    GlaxnimateApp app(argc, argv);

    app.initialize();

    GlaxnimateWindow window;
    window.show();
    int ret = app.exec();

    app.finalize();

    return ret;
}
