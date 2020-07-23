#ifndef GLAXNIMATEWINDOW_H
#define GLAXNIMATEWINDOW_H

#include <QMainWindow>
#include <memory>

namespace model { class Document; }

class GlaxnimateWindowPrivate;

class GlaxnimateWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit GlaxnimateWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    ~GlaxnimateWindow();

private slots:
    void document_new();
    void document_save();
    void document_save_as();
    void document_open();

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
    void view_fit();
    void preferences();

protected:
    void changeEvent(QEvent *e) override;
    void showEvent(QShowEvent * event) override;
    void closeEvent ( QCloseEvent * event ) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // GLAXNIMATEWINDOW_H
