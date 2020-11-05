#ifndef GLAXNIMATEWINDOW_H
#define GLAXNIMATEWINDOW_H

#include <QMainWindow>
#include <memory>

#include "model/document.hpp"
#include "model/shapes/shape.hpp"

#include "plugin/executor.hpp"

namespace plugin {
class Plugin;
class PluginScript;
} // namespace plugin

namespace item_models {
class DocumentNodeModel;
} // namespace item_models


namespace tools {
class Tool;
} // namespace tools

namespace model {
class BrushStyle;
} // namespace model

class QItemSelection;

class PluginUiDialog;

class GlaxnimateWindow : public QMainWindow, public plugin::Executor
{
    Q_OBJECT

    Q_PROPERTY(model::Document* document READ document)
    Q_PROPERTY(model::DocumentNode* current_item READ current_document_node WRITE set_current_document_node)
    Q_PROPERTY(model::ShapeElement* current_shape READ current_shape)
    Q_PROPERTY(model::Object* current_shape_container READ current_shape_container_script)
    Q_PROPERTY(QColor fill_color READ current_color WRITE set_current_color)
    Q_PROPERTY(QColor stroke_color READ secondary_color WRITE set_secondary_color)

public:

    explicit GlaxnimateWindow(bool restore_state = true, QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    ~GlaxnimateWindow();

    model::Document* document() const;

    model::Composition* current_composition() const;
    model::DocumentNode* current_document_node() const;
    void set_current_document_node(model::DocumentNode* node);
    model::ShapeElement* current_shape();
    model::ShapeListProperty* current_shape_container();
    model::Object* current_shape_container_script();

    QColor current_color() const;
    void set_current_color(const QColor& c);
    QColor secondary_color() const;
    void set_secondary_color(const QColor& c);

    QPen current_pen_style() const;

    /**
     * @brief Shows a warning popup
     */
    Q_INVOKABLE void warning(const QString& message, const QString& title = "") const;

    /**
     * @brief Shows a message in the status bar
     */
    Q_INVOKABLE void status(const QString& message) const;

    /**
     * \brief Selected nodes, removing nodes that are descendants of other selected nodes
     */
    std::vector<model::DocumentNode*> cleaned_selection() const;

    void delete_selected();
    void group_shapes();
    void ungroup_shapes();
    void move_to();

    item_models::DocumentNodeModel* model() const;

    qreal current_zoom() const;

    void document_open(const QString& filename);

    void switch_tool(tools::Tool* tool);

    /**
     * \brief Shows a file dialog to open an image file
     * \returns The selected name or an empty string if the user canceled the operation
     */
    Q_INVOKABLE QString get_open_image_file(const QString& title, const QString& dir = "") const;

    model::BrushStyle* linked_brush_style(bool secondary) const;

    PluginUiDialog* create_dialog(const QString& ui_file) const;

    bool execute(const plugin::Plugin& plugin, const plugin::PluginScript& script, const QVariantList& in_args) override;
    QVariant get_global(const QString & name) override;

    void trace_dialog(model::ReferenceTarget* object);

public slots:
    void document_save();
    void document_save_as();
    void document_export();
    void document_export_as();
    void view_fit();
    /**
     * \brief Copies the current selection to the clipboard
     */
    void copy() const;
    void paste() const;
    void cut() const;

private slots:
    void document_new();
    void document_open_dialog();
    void document_reload();

    void document_treeview_clicked(const QModelIndex& index);
    void document_treeview_current_changed(const QModelIndex& index);
    void document_treeview_selection_changed(const QItemSelection &selected, const QItemSelection &deselected);
    void scene_selection_changed(const std::vector<model::DocumentNode*>& selected, const std::vector<model::DocumentNode*>& deselected);

    void layer_new_menu();

    void layer_delete();
    void layer_duplicate();
    void layer_top();
    void layer_raise();
    void layer_lower();
    void layer_bottom();

    void help_about();
    void help_manual();
    void help_issue();
    void help_donate();

    void refresh_title();
    void document_open_recent(QAction* action);
    void preferences();
    void web_preview();
    void save_frame_bmp();
    void save_frame_svg();
    void tool_triggered(bool checked);
    void validate_tgs();

    void console_commit(const QString& text);
    void script_reloaded();

protected:
    void changeEvent(QEvent *e) override;
    void showEvent(QShowEvent * event) override;
    void closeEvent ( QCloseEvent * event ) override;
    void timerEvent(QTimerEvent * event) override;

   void dragEnterEvent(QDragEnterEvent* event) override;
   void dragMoveEvent(QDragMoveEvent* event) override;
   void dragLeaveEvent(QDragLeaveEvent* event) override;
   void dropEvent(QDropEvent* event) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // GLAXNIMATEWINDOW_H
