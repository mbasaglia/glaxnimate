/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef KEYBOARDSETTINGSWIDGET_H
#define KEYBOARDSETTINGSWIDGET_H

#include <memory>
#include <QWidget>
#include "app/settings/keyboard_shortcuts.hpp"

class KeyboardSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    KeyboardSettingsWidget(app::settings::ShortcutSettings* settings, QWidget* parent = nullptr);
    ~KeyboardSettingsWidget();

protected:
    void changeEvent ( QEvent* e ) override;

private Q_SLOTS:
    void clear_filter();
    void filter(const QString& text);

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // KEYBOARDSETTINGSWIDGET_H
