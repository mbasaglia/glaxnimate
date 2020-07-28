#include "glaxnimate_app.hpp"
#include "ui/dialogs/glaxnimate_window.hpp"

#include "ui/widgets/keyframe_editor_widget.hpp"
int main(int argc, char *argv[])
{
    GlaxnimateApp app(argc, argv);

    app.initialize();

    GlaxnimateWindow window;
    window.show();
    model::KeyframeTransition t;
    KeyframeEditorWidget wid;
    wid.set_target(&t);
    wid.show();
    int ret = app.exec();

    app.finalize();

    return ret;
}
