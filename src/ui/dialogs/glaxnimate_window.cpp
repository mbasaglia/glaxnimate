#include "glaxnimate_window.hpp"
#include "glaxnimate_window_p.hpp"


GlaxnimateWindow::GlaxnimateWindow(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags), d(std::make_unique<Private>())
{
    d->setupUi(this);
    document_new();
}

GlaxnimateWindow::~GlaxnimateWindow() = default;


void GlaxnimateWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            d->retranslateUi(this);
            break;
        default:
            break;
    }
}

bool GlaxnimateWindow::eventFilter(QObject* object, QEvent* event)
{
    return d->eventFilter(object, event);
}

void GlaxnimateWindow::document_new()
{
    model::Document* curr = d->current_document();

    model::Document* new_doc = d->create_document(tr("New Animation"));

    if ( curr )
    {
        auto opts = new_doc->export_options();
        opts.path = curr->export_options().path;
        new_doc->set_export_options(opts);
    }
}

void GlaxnimateWindow::document_save()
{
    if ( d->save_document(d->current_document(), false, true) )
        d->ui.status_bar->showMessage(tr("File saved"), 5000);
    else
        d->ui.status_bar->showMessage(tr("Could not save file"));
}

void GlaxnimateWindow::document_save_as()
{
    if ( d->save_document(d->current_document(), true, true) )
        d->ui.status_bar->showMessage(tr("File saved"), 5000);
    else
        d->ui.status_bar->showMessage(tr("Could not save file"));
}


