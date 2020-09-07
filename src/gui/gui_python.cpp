#include "widgets/dialogs/glaxnimate_window.hpp"
#include "app/scripting/python/register_machinery.hpp"

PYBIND11_EMBEDDED_MODULE(glaxnimate_gui, m)
{
    using namespace app::scripting::python;
    register_from_meta<GlaxnimateWindow, QObject>(m);
}
