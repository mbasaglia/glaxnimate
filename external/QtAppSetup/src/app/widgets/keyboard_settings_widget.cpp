/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "keyboard_settings_widget.hpp"
#include "ui_keyboard_settings_widget.h"

#include <QEvent>

#include "app/settings/keyboard_shortcuts_model.hpp"

class KeyboardSettingsWidget::Private
{
public:
    Private(app::settings::ShortcutSettings* settings)
        : inner_model(settings), settings(settings)
    {
        model.setSourceModel(&inner_model);
        model.setFilterCaseSensitivity(Qt::CaseInsensitive);
    }

    Ui::KeyboardSettingsWidget ui;
    app::settings::KeyboardShortcutsModel inner_model;
    app::settings::KeyboardShortcutsFilterModel model;
    app::settings::KeyboardShortcutsDelegate delegate;
    app::settings::ShortcutSettings* settings;

};

KeyboardSettingsWidget::KeyboardSettingsWidget(app::settings::ShortcutSettings* settings, QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>(settings))
{
    d->ui.setupUi(this);
    d->ui.tree_view->setModel(&d->model);
    d->ui.tree_view->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    d->ui.tree_view->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    d->ui.tree_view->setItemDelegateForColumn(1, &d->delegate);

    connect(settings, &app::settings::ShortcutSettings::begin_actions_change, &d->inner_model, &app::settings::KeyboardShortcutsModel::begin_change_data);
    connect(settings, &app::settings::ShortcutSettings::end_actions_change, &d->inner_model, &app::settings::KeyboardShortcutsModel::end_change_data);
}

KeyboardSettingsWidget::~KeyboardSettingsWidget() = default;

void KeyboardSettingsWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void KeyboardSettingsWidget::clear_filter()
{
    d->ui.filter->setText({});
}

void KeyboardSettingsWidget::filter(const QString& text)
{
    d->model.setFilterFixedString(text);
}
