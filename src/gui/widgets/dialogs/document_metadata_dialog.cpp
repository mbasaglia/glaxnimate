/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "document_metadata_dialog.hpp"
#include "ui_document_metadata_dialog.h"
#include <QEvent>


using namespace glaxnimate::gui;
using namespace glaxnimate;

class DocumentMetadataDialog::Private
{
public:
    Ui::DocumentMetadataDialog ui;
    model::Document* document;

    void add_placeholder()
    {
        int row = ui.table_widget->rowCount() - 1;

        auto name = new QTableWidgetItem("");
        ui.table_widget->setItem(row, 0, name);

        auto value = new QTableWidgetItem();
        value->setData(Qt::DisplayRole, "");
        value->setData(Qt::EditRole, "");
        ui.table_widget->setItem(row, 1, value);
    }
};

DocumentMetadataDialog::DocumentMetadataDialog(model::Document* document, QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
    d->document = document;

    d->ui.table_widget->setRowCount(d->document->metadata().size() + 1);
    int row = 0;
    for ( auto it = d->document->metadata().begin(); it != d->document->metadata().end(); ++it, row++ )
    {
        auto name = new QTableWidgetItem(it.key());

        auto value = new QTableWidgetItem();
        value->setData(Qt::DisplayRole, *it);
        value->setData(Qt::EditRole, *it);


        d->ui.table_widget->setItem(row, 0, name);
        d->ui.table_widget->setItem(row, 1, value);
    }

    d->ui.edit_author->setText(d->document->info().author);
    d->ui.edit_description->setText(d->document->info().description);
    d->ui.edit_keywords->setPlainText(d->document->info().keywords.join("\n"));

    d->add_placeholder();
}

DocumentMetadataDialog::~DocumentMetadataDialog() = default;

void DocumentMetadataDialog::changeEvent ( QEvent* e )
{
    QDialog::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void DocumentMetadataDialog::update_item(QTableWidgetItem* item)
{
    int row = d->ui.table_widget->row(item);
    if ( row == d->ui.table_widget->rowCount() - 1 )
    {
        if ( !item->data(Qt::EditRole).toString().isEmpty() )
        {
            d->ui.table_widget->setRowCount(d->ui.table_widget->rowCount() + 1);
            d->add_placeholder();
        }
    }
    else
    {
        int column = d->ui.table_widget->column(item);
        if ( column == 0 && item->data(Qt::EditRole).toString().isEmpty() )
        {
            d->ui.table_widget->removeRow(d->ui.table_widget->row(item));
        }
    }
}

void DocumentMetadataDialog::button_clicked(QAbstractButton* button)
{
    if ( d->ui.button_box->buttonRole(button) == QDialogButtonBox::AcceptRole )
    {
        QVariantMap meta;
        for ( int row = 0; row < d->ui.table_widget->rowCount() - 1; row++ )
        {
            QString name = d->ui.table_widget->item(row, 0)->data(Qt::EditRole).toString();
            if ( name.isEmpty() )
                continue;

            meta[name] = d->ui.table_widget->item(row, 1)->data(Qt::EditRole);
        }

        /// \todo undo commands (and macro)
        d->document->set_metadata(meta);
        d->document->info().author = d->ui.edit_author->text();
        d->document->info().description = d->ui.edit_description->text();
        d->document->info().keywords = d->ui.edit_keywords->toPlainText().split("\n");
        accept();
    }
    else
    {
        reject();
    }
}

