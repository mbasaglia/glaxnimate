#ifndef EXPORTIMAGESEQUENCEDIALOG_H
#define EXPORTIMAGESEQUENCEDIALOG_H

#include <memory>
#include <QDialog>
#include "model/document.hpp"

namespace glaxnimate::gui {

class ExportImageSequenceDialog : public QDialog
{
    Q_OBJECT

public:
    ExportImageSequenceDialog(model::Document* document, QDir export_path, QWidget* parent = nullptr);
    ~ExportImageSequenceDialog();

    QDir export_path() const;

protected:
    void changeEvent ( QEvent* e ) override;

private slots:
    void pick_path();
    void render();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui

#endif // EXPORTIMAGESEQUENCEDIALOG_H
