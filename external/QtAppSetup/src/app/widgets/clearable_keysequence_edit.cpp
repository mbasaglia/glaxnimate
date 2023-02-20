/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "clearable_keysequence_edit.hpp"
#include "ui_clearable_keysequence_edit.h"
#include <QEvent>

class ClearableKeysequenceEdit::Private
{
public:
    Ui::ClearableKeysequenceEdit ui;
    QKeySequence default_ks;
};

ClearableKeysequenceEdit::ClearableKeysequenceEdit(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
}

ClearableKeysequenceEdit::~ClearableKeysequenceEdit() = default;

void ClearableKeysequenceEdit::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void ClearableKeysequenceEdit::set_key_sequence(const QKeySequence& ks)
{
    d->ui.sequence_edit->setKeySequence(ks);
}

void ClearableKeysequenceEdit::set_default_key_sequence(const QKeySequence& ks)
{
    d->default_ks = ks;
}

QKeySequence ClearableKeysequenceEdit::key_sequence() const
{
    return d->ui.sequence_edit->keySequence();
}

void ClearableKeysequenceEdit::use_default()
{
    d->ui.sequence_edit->setKeySequence(d->default_ks);
}

void ClearableKeysequenceEdit::use_nothing()
{
    d->ui.sequence_edit->setKeySequence({});
}
