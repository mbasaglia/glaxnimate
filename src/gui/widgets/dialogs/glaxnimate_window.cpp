#include "glaxnimate_window.hpp"
#include "glaxnimate_window_p.hpp"

#include <QDesktopServices>
#include <QCloseEvent>
#include <QDragEnterEvent>

#include "app/widgets/settings_dialog.hpp"
#include "app_info.hpp"

GlaxnimateWindow::GlaxnimateWindow(bool restore_state, QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags), d(std::make_unique<Private>())
{
    d->setupUi(restore_state, this);
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
    if ( d->save_document(false, false) )
        d->status_message(tr("File saved"));
    else
        d->status_message(tr("Could not save file"), 0);
}

void GlaxnimateWindow::document_save_as()
{
    if ( d->save_document(true, false) )
        d->status_message(tr("File saved"));
    else
        d->status_message(tr("Could not save file"), 0);
}

void GlaxnimateWindow::document_export()
{
    if ( d->save_document(false, true) )
        d->status_message(tr("File exported"));
    else
        d->status_message(tr("Could not export file"), 0);
}

void GlaxnimateWindow::document_export_as()
{
    if ( d->save_document(true, true) )
        d->status_message(tr("File exported"));
    else
        d->status_message(tr("Could not export file"), 0);
}

void GlaxnimateWindow::document_open_dialog()
{
    d->document_open();
}

void GlaxnimateWindow::document_treeview_clicked ( const QModelIndex& index )
{
    auto node = d->document_node_model.node(d->comp_model.mapToSource(index));
    if ( !node )
        return;

    if ( index.column() == item_models::DocumentNodeModel::ColumnVisible )
        node->visible.set(!node->visible.get());
     else if ( index.column() == item_models::DocumentNodeModel::ColumnLocked )
        node->locked.set(!node->locked.get());
}

void GlaxnimateWindow::document_treeview_current_changed(const QModelIndex& index)
{
    d->document_treeview_current_changed(index);
}


void GlaxnimateWindow::layer_new_menu()
{
    d->layer_new_layer();
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

void GlaxnimateWindow::help_donate()
{
    QDesktopServices::openUrl(AppInfo::instance().url_donate());
}

model::Composition * GlaxnimateWindow::current_composition() const
{
    return d->current_composition();
}

model::DocumentNode * GlaxnimateWindow::current_document_node() const
{
    return d->current_document_node();
}

QColor GlaxnimateWindow::current_color() const
{
    return d->ui.fill_style_widget->current_color();
}

QColor GlaxnimateWindow::secondary_color() const
{
    return d->ui.fill_style_widget->secondary_color();
}

void GlaxnimateWindow::set_current_color(const QColor& c)
{
    d->ui.fill_style_widget->set_current_color(c);
}

void GlaxnimateWindow::set_secondary_color(const QColor& c)
{
    d->ui.stroke_style_widget->set_color(c);
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
    d->paste(false);
}

void GlaxnimateWindow::cut() const
{
    d->cut();
}

void GlaxnimateWindow::duplicate_selection() const
{
    d->duplicate_selection();
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

void GlaxnimateWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if ( !d->drop_event_data(event).isEmpty() )
        event->acceptProposedAction();
}

void GlaxnimateWindow::dragLeaveEvent(QDragLeaveEvent* event)
{
    event->accept();
}

void GlaxnimateWindow::dragMoveEvent(QDragMoveEvent* event)
{
    if ( !d->drop_event_data(event).isEmpty() )
        event->acceptProposedAction();
}

void GlaxnimateWindow::dropEvent(QDropEvent* event)
{
    auto str = d->drop_event_data(event);
    if ( !str.isEmpty() )
        document_open(str);
}

void GlaxnimateWindow::switch_tool(tools::Tool* tool)
{
    d->switch_tool(tool);
}

QString GlaxnimateWindow::get_open_image_file(const QString& title, const QString& dir) const
{
    return d->get_open_image_file(title, dir);
}

model::BrushStyle * GlaxnimateWindow::linked_brush_style ( bool secondary ) const
{
    if ( secondary )
        return d->secondary_brush;
    return d->main_brush;
}

PluginUiDialog * GlaxnimateWindow::create_dialog(const QString& ui_file) const
{
    return d->ui.console->create_dialog(ui_file);
}

void GlaxnimateWindow::trace_dialog(model::ReferenceTarget* object)
{
    return d->trace_dialog(object);
}

void GlaxnimateWindow::shape_to_precomposition(model::ShapeElement* node)
{
    return d->shape_to_precomposition(node);
}

void GlaxnimateWindow::set_current_composition(model::Composition* comp)
{
    int index = d->current_document->defs()->precompositions.index_of(static_cast<model::Precomposition*>(comp));
    d->ui.tab_bar->setCurrentIndex(index+1);
}

QMenu * GlaxnimateWindow::create_layer_menu() const
{
    return d->ui.menu_new_layer;
}

void GlaxnimateWindow::select(const std::vector<model::DocumentNode *>& nodes)
{
    d->scene.user_select(nodes, graphics::DocumentScene::Replace);
}
