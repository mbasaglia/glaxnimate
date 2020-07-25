#include "glaxnimate_window.hpp"
#include "glaxnimate_window_p.hpp"
#include "ui/dialogs/settings_dialog.hpp"

#include <QCloseEvent>


GlaxnimateWindow::GlaxnimateWindow(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags), d(std::make_unique<Private>())
{
    d->setupUi(this);
    d->setup_document_new(tr("New Animation"));
}

GlaxnimateWindow::~GlaxnimateWindow() = default;


void GlaxnimateWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
            d->retranslateUi(this);
            break;
        default:
            break;
    }
}

void GlaxnimateWindow::document_new()
{
    d->setup_document_new(tr("New Animation"));
}

void GlaxnimateWindow::document_save()
{
    if ( d->save_document(false, true) )
        d->ui.status_bar->showMessage(tr("File saved"), 5000);
    else
        d->ui.status_bar->showMessage(tr("Could not save file"));
}

void GlaxnimateWindow::document_save_as()
{
    if ( d->save_document(true, true) )
        d->ui.status_bar->showMessage(tr("File saved"), 5000);
    else
        d->ui.status_bar->showMessage(tr("Could not save file"));
}

void GlaxnimateWindow::document_open_dialog()
{
    d->document_open();
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

void GlaxnimateWindow::document_treeview_current_changed(const QModelIndex& index)
{
    d->document_treeview_current_changed(index);
}


void GlaxnimateWindow::layer_new_menu()
{
    layer_new_shape();
}

void GlaxnimateWindow::layer_new_empty()
{
    d->layer_new<model::EmptyLayer>();
}

void GlaxnimateWindow::layer_new_precomp()
{
//     d->layer_new<model::PrecompLayer>();
}

void GlaxnimateWindow::layer_new_shape()
{
    d->layer_new<model::ShapeLayer>();
}

void GlaxnimateWindow::refresh_title()
{
    d->refresh_title();
}

void GlaxnimateWindow::layer_delete()
{
    d->layer_delete();
}

void GlaxnimateWindow::view_fit()
{
    d->view_fit();
}

void GlaxnimateWindow::showEvent(QShowEvent * event)
{
    QMainWindow::showEvent(event);
    if ( !d->started )
    {
        d->started = true;
        d->view_fit();
    }
}

void GlaxnimateWindow::preferences()
{
    SettingsDialog(this).exec();
}

void GlaxnimateWindow::closeEvent ( QCloseEvent* event )
{
    if ( !d->close_document() )
    {
        event->ignore();
    }
    else
    {
        d->shutdown();
        QMainWindow::closeEvent(event);
    }
}

void GlaxnimateWindow::document_open_recent(QAction* action)
{
    d->document_open_from_filename(action->data().toString());
}

void GlaxnimateWindow::help_about()
{
    d->help_about();
}

void GlaxnimateWindow::console_commit()
{
    d->console_commit();
}

void GlaxnimateWindow::document_open(const QString& filename)
{
    d->document_open_from_filename(filename);
}

model::Document * GlaxnimateWindow::document() const
{
    return d->current_document.get();
}
