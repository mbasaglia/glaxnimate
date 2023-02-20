/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DOCUMENTMETADATADIALOG_H
#define DOCUMENTMETADATADIALOG_H

#include <memory>
#include <QDialog>
#include <QTableWidgetItem>
#include <QAbstractButton>

#include "model/document.hpp"

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
