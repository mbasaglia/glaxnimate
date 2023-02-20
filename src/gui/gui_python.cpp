/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "widgets/dialogs/glaxnimate_window.hpp"
#include "widgets/dialogs/plugin_ui_dialog.hpp"
#include "app/scripting/python/register_machinery.hpp"

PYBIND11_EMBEDDED_MODULE(glaxnimate_gui, m)
{
    using namespace app::scripting::python;
    using namespace glaxnimate::gui;

    register_from_meta<PluginUiDialog, QObject>(m)
        .def("exec", &QDialog::exec)
    ;
    register_from_meta<GlaxnimateWindow, QObject>(m)
        .def("create_dialog", &GlaxnimateWindow::create_dialog)
        .def_property_readonly("cleaned_selection", &GlaxnimateWindow::cleaned_selection, py::return_value_policy::automatic_reference)
        .def_property_readonly("select", &GlaxnimateWindow::select, py::return_value_policy::automatic_reference)
        .def_property("size", &GlaxnimateWindow::size, qOverload<const QSize&>(&GlaxnimateWindow::resize))
        .def("resize", qOverload<int, int>(&GlaxnimateWindow::resize))
    ;
}
