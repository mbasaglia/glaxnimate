#ifndef GLAXNIMATEWINDOW_H
#define GLAXNIMATEWINDOW_H

#include <QMainWindow>
#include <memory>

#include "model/document.hpp"

namespace model { class Document; }

namespace app::scripting {

class Plugin;
class PluginScript;

} // namespace app::scripting

class QItemSelection;

class GlaxnimateWindow : public QMainWindow
{
    Q_OBJECT

    Q_PROPERTY(model::Document* document READ document)

public:

    explicit GlaxnimateWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    ~GlaxnimateWindow();

    model::Document* document() const;

    /**
     * @brief Shows a warning popup
     */
    Q_INVOKABLE void warning(const QString& message, const QString& title = "123") const;

    /**
     * @brief Shows a message in the status bar
     */
    Q_INVOKABLE void status(const QString& message) const;

public slots:
    void document_save();
    void document_save_as();
    void view_fit();

private slots:
    void document_new();
    void document_open_dialog();
    void document_open(const QString& filename);
    void color_update_noalpha(const QColor& col);
    void color_update_alpha(const QColor& col);
    void color_update_component(int value);
    void color_swap();

    void document_treeview_clicked(const QModelIndex& index);
    void document_treeview_current_changed(const QModelIndex& index);
    void document_treeview_selection_changed(const QItemSelection &selected, const QItemSelection &deselected);

    void layer_new_menu();
    void layer_new_empty();
    void layer_new_shape();
    void layer_new_precomp();
    void layer_new_color();
    void layer_delete();

    void refresh_title();
    void document_open_recent(QAction* action);
    void preferences();
    void help_about();
    void web_preview();
    void save_frame_bmp();
    void save_frame_svg();

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
