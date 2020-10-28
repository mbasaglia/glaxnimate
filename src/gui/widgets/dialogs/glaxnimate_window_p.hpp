#ifndef GLAXNIMATEWINDOW_P_H
#define GLAXNIMATEWINDOW_P_H

#include <QToolButton>
#include <QMessageBox>
#include <QPointer>

#include "QtColorWidgets/color_delegate.hpp"
#include "QtColorWidgets/color_palette_model.hpp"

#include "ui_glaxnimate_window.h"
#include "glaxnimate_window.hpp"

#include "model/document.hpp"
#include "command/structure_commands.hpp"

#include "graphics/document_scene.hpp"
#include "item_models/document_node_model.hpp"
#include "item_models/property_model.hpp"

#include "style/dock_widget_style.hpp"
#include "style/property_delegate.hpp"

#include "app/settings/settings.hpp"
#include "app/scripting/script_engine.hpp"
#include "plugin/plugin.hpp"
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
    io::Options export_options;
    bool current_document_has_file = false;

    std::vector<app::scripting::ScriptContext> script_contexts;

    QPointer<model::BrushStyle> main_brush;
    QPointer<model::BrushStyle> secondary_brush;

    tools::Tool* active_tool = nullptr;
    std::map<QString, std::vector<QWidget*>> tool_widgets;
    std::map<QString, std::vector<QAction*>> tool_actions;

    // "set and forget" kida variables
    int autosave_timer = 0;
    int autosave_timer_mins = 0;
    bool autosave_load = false;
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
    color_widgets::ColorPaletteModel palette_model;

    // document
    void setup_document(const QString& filename);
    void setup_document_new(const QString& filename);
    bool setup_document_open(const io::Options& options);
    void refresh_title();
    bool close_document();
    bool save_document(bool force_dialog, bool export_opts);
    void document_open();
    void document_open_from_filename(const QString& filename);
    void document_reload();
    void web_preview();
    void save_frame_bmp();
    void save_frame_svg();
    void validate_tgs();
    void autosave_timer_load_settings();
    void autosave_timer_start(int mins = -1);
    void autosave_timer_tick();
    QString backup_name();
    QString backup_name(const QUuid& uuid);
    void load_backup(const QUuid& uuid);
    QString drop_event_data(QDropEvent* ev);
    void import_image();

    // ui
    void setupUi(bool restore_state, GlaxnimateWindow* parent);
    void retranslateUi(QMainWindow* parent);
    void view_fit();
    void document_treeview_current_changed(const QModelIndex& index);
    void reload_recent_menu();
    void most_recent_file(const QString& s);
    void show_warning(const QString& title, const QString& message, app::log::Severity icon = app::log::Warning);
    void help_about();
    void shutdown();
    void document_treeview_selection_changed(const QItemSelection &selected, const QItemSelection &deselected);
    void scene_selection_changed(const std::vector<model::DocumentNode*>& selected, const std::vector<model::DocumentNode*>& deselected);
    void switch_tool(tools::Tool* tool);
    void switch_tool_action(QAction* action);
    void status_message(const QString& msg, int duration=5000);
    void set_color_def(model::BrushStyle* sty, bool secondary);
    QString get_open_image_file(const QString& title, const QString& dir);
    void set_brush_reference(model::BrushStyle* sty, bool secondary);

    // Model
    model::Composition* current_composition();
    model::DocumentNode* current_document_node();
    void set_current_document_node(model::DocumentNode* node);
    model::ShapeElement* current_shape();
    model::ShapeListProperty* current_shape_container();
    std::vector<model::DocumentNode*> cleaned_selection();
    std::vector<model::DocumentNode*> copy();
    void paste();
    void cut();
    void delete_selected();
    void move_current(command::ReorderCommand::SpecialPosition pos);
    void group_shapes();
    void ungroup_shapes();
    void move_to();
    void cleanup_document();

    void layer_new_layer();
    void layer_new_fill();
    void layer_new_stroke();
    void layer_new_group();
    void layer_new_impl(std::unique_ptr<model::ShapeElement> layer);
    void layer_delete();
    void layer_duplicate();

    // script
    void console_error(const app::scripting::ScriptError& err);
    void console_stderr(const QString& line);
    void console_stdout(const QString& line);
    void console_commit(QString text);
    bool ensure_script_contexts();
    void create_script_context();
    void script_needs_running ( const plugin::Plugin& plugin, const plugin::PluginScript& script, const QVariantMap& settings );
};

#endif // GLAXNIMATEWINDOW_P_H
