#ifndef DOCUMENTMETADATADIALOG_H
#define DOCUMENTMETADATADIALOG_H

#include <memory>
#include <QDialog>
#include <QTableWidgetItem>
#include <QAbstractButton>

#include "glaxnimate/core/model/document.hpp"

namespace glaxnimate::gui {

class DocumentMetadataDialog : public QDialog
{
    Q_OBJECT

public:
    DocumentMetadataDialog(model::Document* document, QWidget* parent = nullptr);
    ~DocumentMetadataDialog();

protected:
    void changeEvent ( QEvent* e ) override;

private slots:
    void update_item(QTableWidgetItem*);
    void button_clicked(QAbstractButton*);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui

#endif // DOCUMENTMETADATADIALOG_H
