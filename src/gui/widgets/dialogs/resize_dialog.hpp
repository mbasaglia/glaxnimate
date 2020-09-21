#ifndef RESIZEDIALOG_H
#define RESIZEDIALOG_H

#include <QDialog>
#include <memory>

#include "model/document.hpp"

class ResizeDialog : public QDialog
{
    Q_OBJECT

public:
    ResizeDialog(QWidget* parent = nullptr);

    ~ResizeDialog();

    void resize_document(model::Document* doc);

protected:
    void changeEvent ( QEvent* e ) override;

private slots:
    void width_changed(int w);
    void height_changed(int h);
    void lock_changed(bool locked);

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // RESIZEDIALOG_H
