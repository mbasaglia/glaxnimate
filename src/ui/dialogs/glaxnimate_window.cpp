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

void GlaxnimateWindow::color_update_alpha ( const QColor& col )
{
    d->update_color(col, true, QObject::sender());
}

void GlaxnimateWindow::color_update_noalpha ( const QColor& col )
{

    d->update_color(col, false, QObject::sender());
}

void GlaxnimateWindow::color_update_component ( int value )
{
    d->update_color_component(value, QObject::sender());
}

void GlaxnimateWindow::document_treeview_clicked ( const QModelIndex& index )
{
    auto node = d->document_node_model.node(index);
    if ( !node )
        return;

    if ( index.column() == model::DocumentNodeModel::ColumnVisible )
        node->docnode_set_visible(!node->docnode_visible());
     else if ( index.column() == model::DocumentNodeModel::ColumnLocked )
        node->docnode_set_locked(!node->docnode_locked());
}

void GlaxnimateWindow::layer_new_menu()
{
    d->ui.menu_new_layer->popup(QCursor::pos(), d->ui.action_new_layer_shape);
}

void GlaxnimateWindow::layer_new_null()
{
}

void GlaxnimateWindow::layer_new_precomp()
{
}

void GlaxnimateWindow::layer_new_shape()
{
}

void GlaxnimateWindow::refresh_title(model::Document* doc)
{
    d->refresh_title(doc);
}



