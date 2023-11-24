/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "stalefiles_dialog.hpp"
#include "ui_stalefiles_dialog.h"
#include <QEvent>
#include <QFileInfo>

using namespace glaxnimate::gui;

class StalefilesDialog::Private
{
public:
    Ui::StalefilesDialog ui;
    QList<KAutoSaveFile*> stale;
    KAutoSaveFile* current = nullptr;
    int index = -1;
};

StalefilesDialog::StalefilesDialog(const QList<KAutoSaveFile*>& stale, QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
    d->stale = stale;
    for ( const auto& file : stale )
    {
        QFileInfo finfo(file->managedFile().toString());
        auto item = new QListWidgetItem(finfo.fileName(), d->ui.list_widget);
        item->setToolTip(finfo.filePath());
    }
}

StalefilesDialog::~StalefilesDialog() = default;

void StalefilesDialog::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void glaxnimate::gui::StalefilesDialog::current_changed(int index)
{
    d->index = index;
    if ( index >= 0 && index < d->stale.size() )
        d->current = d->stale[index];
    else
        d->current = nullptr;

    d->ui.button_open->setEnabled(d->current);
    d->ui.button_delete->setEnabled(d->current);
}

void glaxnimate::gui::StalefilesDialog::delete_all()
{
    for ( auto stale : d->stale )
    {
        stale->remove();
        delete stale;
    }
    d->index = -1;
    d->current = nullptr;
    d->stale.clear();
    reject();
}

void glaxnimate::gui::StalefilesDialog::delete_selected()
{
    if ( d->current )
    {
        d->current->remove();
        d->stale.removeAt(d->index);
        delete d->current;
        int row = d->index;
        d->index = -1;
        d->current = nullptr;
        d->ui.list_widget->takeItem(row);
        if ( d->stale.size() == 0 )
            reject();
    }
}

KAutoSaveFile * glaxnimate::gui::StalefilesDialog::selected() const
{
    return d->current;
}


void glaxnimate::gui::StalefilesDialog::cleanup(KAutoSaveFile* keep)
{
    for ( auto stale : d->stale )
    {
        if ( stale != keep )
            delete stale;
    }
}
