#include "glaxnimate_window.hpp"
#include "glaxnimate_window_p.hpp"

#include <QDesktopServices>
#include <QCloseEvent>

#include "app/widgets/settings_dialog.hpp"
#include "app_info.hpp"
#include "model/layers/layers.hpp"


GlaxnimateWindow::GlaxnimateWindow(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags), d(std::make_unique<Private>())
{
    d->setupUi(this);
    d->setup_document_new(tr("New Animation"));
    d->autosave_timer_start();
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
        d->status_message(tr("File saved"));
    else
        d->status_message(tr("Could not save file"), 0);
}

void GlaxnimateWindow::document_save_as()
{
    if ( d->save_document(true, true) )
        d->status_message(tr("File saved"));
    else
        d->status_message(tr("Could not save file"), 0);
}

void GlaxnimateWindow::document_open_dialog()
{
    d->document_open();
}

void GlaxnimateWindow::document_treeview_clicked ( const QModelIndex& index )
{
    auto node = d->document_node_model.node(index);
    if ( !node )
        return;

    if ( index.column() == item_models::DocumentNodeModel::ColumnVisible )
        node->docnode_set_visible(!node->docnode_visible());
     else if ( index.column() == item_models::DocumentNodeModel::ColumnLocked )
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

void GlaxnimateWindow::layer_duplicate()
{
    d->layer_duplicate();
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
    app::SettingsDialog(this).exec();
    d->autosave_timer_load_settings();
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

void GlaxnimateWindow::console_commit(const QString& text)
{
    d->console_commit(text);
}

void GlaxnimateWindow::document_open(const QString& filename)
{
    d->document_open_from_filename(filename);
}

void GlaxnimateWindow::document_reload()
{
    d->document_reload();
}

model::Document * GlaxnimateWindow::document() const
{
    return d->current_document.get();
}

void GlaxnimateWindow::warning ( const QString& message, const QString& title ) const
{
    d->show_warning(title.isEmpty() ? tr("Warning") : title, message);
}

void GlaxnimateWindow::status ( const QString& message ) const
{
    d->status_message(message);
}


void GlaxnimateWindow::script_needs_running ( const app::scripting::Plugin& plugin, const app::scripting::PluginScript& script, const QVariantMap& settings )
{
    d->script_needs_running(plugin, script, settings);
}

void GlaxnimateWindow::script_reloaded()
{
    d->script_contexts.clear();
}

void GlaxnimateWindow::layer_new_color()
{
    d->layer_new<model::SolidColorLayer>();
}

void GlaxnimateWindow::web_preview()
{
    d->web_preview();
}

void GlaxnimateWindow::save_frame_bmp()
{
    d->save_frame_bmp();
}

void GlaxnimateWindow::save_frame_svg()
{
    d->save_frame_svg();
}

void GlaxnimateWindow::document_treeview_selection_changed(const QItemSelection &selected, const QItemSelection &deselected)
{
    d->document_treeview_selection_changed(selected, deselected);
}

void GlaxnimateWindow::scene_selection_changed(const std::vector<model::DocumentNode*>& selected, const std::vector<model::DocumentNode*>& deselected)
{
    d->scene_selection_changed(selected, deselected);
}

void GlaxnimateWindow::tool_triggered(bool checked)
{
    if ( checked )
        d->switch_tool_action(static_cast<QAction*>(sender()));
}

void GlaxnimateWindow::help_manual()
{
    QDesktopServices::openUrl(AppInfo::instance().url_docs());
}

void GlaxnimateWindow::help_issue()
{
    QDesktopServices::openUrl(AppInfo::instance().url_issues());
}

model::Composition * GlaxnimateWindow::current_composition() const
{
    return d->current_composition();
}

model::DocumentNode * GlaxnimateWindow::current_document_node() const
{
    return d->current_document_node();
}

model::Layer * GlaxnimateWindow::current_layer() const
{
    return d->current_layer();
}

QColor GlaxnimateWindow::current_color() const
{
    return d->ui.color_selector->current_color();
}

QColor GlaxnimateWindow::secondary_color() const
{
    return d->ui.color_selector->secondary_color();
}

void GlaxnimateWindow::set_current_color(const QColor& c)
{
    d->ui.color_selector->set_current_color(c);
}

void GlaxnimateWindow::set_secondary_color(const QColor& c)
{
    d->ui.color_selector->set_secondary_color(c);
}

model::ShapeElement * GlaxnimateWindow::current_shape()
{
    return d->current_shape();
}

model::ShapeListProperty * GlaxnimateWindow::current_shape_container()
{
    return d->current_shape_container();
}

model::Object * GlaxnimateWindow::current_shape_container_script()
{
    auto prop = d->current_shape_container();
    return prop ? prop->object() : nullptr;
}


void GlaxnimateWindow::set_current_document_node(model::DocumentNode* node)
{
    d->set_current_document_node(node);
}

QPen GlaxnimateWindow::current_pen_style() const
{
    return d->ui.stroke_style_widget->pen_style();
}

std::vector<model::DocumentNode *> GlaxnimateWindow::cleaned_selection() const
{
    return d->cleaned_selection();
}

void GlaxnimateWindow::copy() const
{
    d->copy();
}

void GlaxnimateWindow::paste() const
{
    d->paste();
}

void GlaxnimateWindow::cut() const
{
    d->cut();
}

void GlaxnimateWindow::delete_selected()
{
    d->delete_selected();
}

void GlaxnimateWindow::layer_top()
{
    d->move_current(command::ReorderCommand::MoveTop);
}

void GlaxnimateWindow::layer_raise()
{
    d->move_current(command::ReorderCommand::MoveUp);
}

void GlaxnimateWindow::layer_lower()
{
    d->move_current(command::ReorderCommand::MoveDown);
}

void GlaxnimateWindow::layer_bottom()
{
    d->move_current(command::ReorderCommand::MoveBottom);
}

item_models::DocumentNodeModel * GlaxnimateWindow::model() const
{
    return &d->document_node_model;
}

void GlaxnimateWindow::group_shapes()
{
    d->group_shapes();
}

void GlaxnimateWindow::ungroup_shapes()
{
    d->ungroup_shapes();
}

void GlaxnimateWindow::move_to()
{
    d->move_to();
}

void GlaxnimateWindow::validate_tgs()
{
    d->validate_tgs();
}

qreal GlaxnimateWindow::current_zoom() const
{
    return d->ui.graphics_view->get_zoom_factor();
}

void GlaxnimateWindow::timerEvent(QTimerEvent*)
{
    d->autosave_timer_tick();
}
