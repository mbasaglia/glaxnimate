#include "glaxnimate_window.hpp"
#include "glaxnimate_window_p.hpp"

GlaxnimateWindow::GlaxnimateWindow()
    : d_ptr(new GlaxnimateWindowPrivate())
{
    d_ptr->ui.setupUi(this);
}

GlaxnimateWindow::~GlaxnimateWindow()
{
    delete d_ptr;
}
