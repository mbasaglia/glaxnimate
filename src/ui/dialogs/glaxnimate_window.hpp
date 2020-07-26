#ifndef GLAXNIMATEWINDOW_H
#define GLAXNIMATEWINDOW_H

#include <QMainWindow>
#include <memory>

#include "model/document.hpp"

namespace model { class Document; }

class GlaxnimateWindowPrivate;

#include <QDebug>
class GlaxnimateWindow : public QMainWindow
{
    Q_OBJECT

    Q_PROPERTY(model::Document* document READ document)

    using TestType = QVariant;
    Q_PROPERTY(QVariant foo READ foo WRITE set_foo)
public:
    TestType foo() const { return foo_; } void set_foo(const TestType& v) { foo_ = v; qDebug() << v; } TestType foo_;

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
    void document_new();
    void document_save();
    void document_save_as();
    void document_open_dialog();
    void document_open(const QString& filename);
    void view_fit();

private slots:
    void color_update_noalpha(const QColor& col);
    void color_update_alpha(const QColor& col);
    void color_update_component(int value);

    void document_treeview_clicked(const QModelIndex& index);
    void document_treeview_current_changed(const QModelIndex& index);

    void layer_new_menu();
    void layer_new_empty();
    void layer_new_shape();
    void layer_new_precomp();
    void layer_delete();

    void refresh_title();
    void preferences();
    void document_open_recent(QAction* action);
    void help_about();
    void console_commit(const QString& text);

protected:
    void changeEvent(QEvent *e) override;
    void showEvent(QShowEvent * event) override;
    void closeEvent ( QCloseEvent * event ) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // GLAXNIMATEWINDOW_H
