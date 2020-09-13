#ifndef GLAXNIMATEWINDOW_P_H
#define GLAXNIMATEWINDOW_P_H

#include <QToolButton>
#include <QMessageBox>

#include "QtColorWidgets/color_delegate.hpp"

#include "ui_glaxnimate_window.h"
#include "glaxnimate_window.hpp"

#include "model/document.hpp"
#include "graphics/document_scene.hpp"

#include "item_models/document_node_model.hpp"
#include "item_models/property_model.hpp"

#include "style/dock_widget_style.hpp"
#include "style/property_delegate.hpp"

#include "app/settings/settings.hpp"
#include "app/scripting/script_engine.hpp"
#include "app/scripting/plugin.hpp"
#include "app/log/log_model.hpp"

class IoStatusDialog;
class AboutDialog;
class ViewTransformWidget;
class FlowLayout;

namespace tools {

class Tool;

} // namespace tools

class GlaxnimateWindow::Private
{
public:
    Ui::GlaxnimateWindow ui;

    std::unique_ptr<model::Document> current_document;

    item_models::DocumentNodeModel document_node_model;
    item_models::PropertyModel property_model;
    graphics::DocumentScene scene;

    GlaxnimateWindow* parent = nullptr;

    QStringList recent_files;

    std::vector<app::scripting::ScriptContext> script_contexts;

    tools::Tool* active_tool = nullptr;

    // "set and forget" kida variables
    int tool_rows = 3; ///< @todo setting
    QString undo_text;
    QString redo_text;
    color_widgets::ColorDelegate color_delegate;
    style::PropertyDelegate property_delegate;
    style::DockWidgetStyle dock_style;
    ViewTransformWidget* view_trans_widget;
    bool started = false;
    IoStatusDialog* dialog_import_status;
    IoStatusDialog* dialog_export_status;
    AboutDialog* about_dialog;
    FlowLayout* dock_tools_layout;
    app::log::LogModel log_model;

    // document
    void setup_document(const QString& filename);
    void setup_document_new(const QString& filename);
    bool setup_document_open(const io::Options& options);
    void refresh_title();
    bool close_document();
    bool save_document(bool force_dialog, bool overwrite_doc);
    void document_open();
    void document_open_from_filename(const QString& filename);
    void web_preview();
    void save_frame_bmp();
    void save_frame_svg();

    // ui
    void setupUi(GlaxnimateWindow* parent);
    void retranslateUi(QMainWindow* parent);
    void view_fit();
    void document_treeview_current_changed(const QModelIndex& index);
    void reload_recent_menu();
    void most_recent_file(const QString& s);
    void show_warning(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Warning);
    void help_about();
    void shutdown();
    void document_treeview_selection_changed(const QItemSelection &selected, const QItemSelection &deselected);
    void scene_selection_changed(const std::vector<model::DocumentNode*>& selected, const std::vector<model::DocumentNode*>& deselected);
    void switch_tool(tools::Tool* tool);
    void switch_tool_action(QAction* action);
    void status_message(const QString& msg, int duration=5000);

    // Model
    model::Composition* current_composition();
    model::Layer* current_layer();
    model::DocumentNode* current_document_node();
    void set_current_document_node(model::DocumentNode* node);
    model::ShapeElement* current_shape();
    model::ShapeListProperty* current_shape_container();
    std::vector<model::DocumentNode*> cleaned_selection();
    std::vector<model::DocumentNode*> copy();
    void paste();
    void cut();
    void delete_selected();

    template<class LayerT>
    void layer_new()
    {
        layer_new_impl(current_composition()->make_layer<LayerT>());
    }
    void layer_new_impl(std::unique_ptr<model::Layer> layer);
    void layer_new_prepare(model::Layer* layer);
    void layer_delete();

    // script
    void console_error(const app::scripting::ScriptError& err);
    void console_stderr(const QString& line);
    void console_stdout(const QString& line);
    void console_commit(QString text);
    bool ensure_script_contexts();
    void create_script_context();
    void script_needs_running ( const app::scripting::Plugin& plugin, const app::scripting::PluginScript& script, const QVariantMap& settings );
};

#endif // GLAXNIMATEWINDOW_P_H
