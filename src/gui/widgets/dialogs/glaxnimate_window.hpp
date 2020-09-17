#ifndef GLAXNIMATEWINDOW_H
#define GLAXNIMATEWINDOW_H

#include <QMainWindow>
#include <memory>

#include "model/document.hpp"
#include "model/shapes/shape.hpp"

namespace app::scripting {

class Plugin;
class PluginScript;

} // namespace app::scripting

namespace item_models {
class DocumentNodeModel;
} // namespace item_models

class QItemSelection;

class GlaxnimateWindow : public QMainWindow
{
    Q_OBJECT

    Q_PROPERTY(model::Document* document READ document)
    Q_PROPERTY(model::Layer* current_layer READ current_layer)
    Q_PROPERTY(model::DocumentNode* current_document_node READ current_document_node WRITE set_current_document_node)
    Q_PROPERTY(model::ShapeElement* current_shape READ current_shape)
    Q_PROPERTY(model::Object* current_shape_container READ current_shape_container_script)
    Q_PROPERTY(QColor current_color READ current_color WRITE set_current_color)
    Q_PROPERTY(QColor secondary_color READ secondary_color WRITE set_secondary_color)

public:

    explicit GlaxnimateWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    ~GlaxnimateWindow();

    model::Document* document() const;

    model::Composition* current_composition() const;
    model::Layer* current_layer() const;
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
    Q_INVOKABLE void warning(const QString& message, const QString& title = "123") const;

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

    item_models::DocumentNodeModel* model() const;

public slots:
    void document_save();
    void document_save_as();
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
    void document_open(const QString& filename);
    void document_reload();

    void document_treeview_clicked(const QModelIndex& index);
    void document_treeview_current_changed(const QModelIndex& index);
    void document_treeview_selection_changed(const QItemSelection &selected, const QItemSelection &deselected);
    void scene_selection_changed(const std::vector<model::DocumentNode*>& selected, const std::vector<model::DocumentNode*>& deselected);

    void layer_new_menu();
    void layer_new_empty();
    void layer_new_shape();
    void layer_new_precomp();
    void layer_new_color();

    void layer_delete();
    void layer_duplicate();
    void layer_top();
    void layer_raise();
    void layer_lower();
    void layer_bottom();

    void help_about();
    void help_manual();
    void help_issue();

    void refresh_title();
    void document_open_recent(QAction* action);
    void preferences();
    void web_preview();
    void save_frame_bmp();
    void save_frame_svg();
    void tool_triggered(bool checked);

    void console_commit(const QString& text);
    void script_needs_running(const app::scripting::Plugin& plugin, const app::scripting::PluginScript& script, const QVariantMap& settings);
    void script_reloaded();

protected:
    void changeEvent(QEvent *e) override;
    void showEvent(QShowEvent * event) override;
    void closeEvent ( QCloseEvent * event ) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // GLAXNIMATEWINDOW_H
