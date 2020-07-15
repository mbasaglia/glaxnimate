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
        new_doc->set_save_path(curr->save_path());
}

