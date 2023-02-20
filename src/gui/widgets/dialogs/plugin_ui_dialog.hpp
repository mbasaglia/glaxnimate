/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#include <QDialog>
#include <QVariant>
#include <QDir>

#include "plugin/plugin.hpp"

namespace glaxnimate::gui {

class PluginUiDialog : public QDialog
{
    Q_OBJECT

public:
    PluginUiDialog(QIODevice& file, const plugin::Plugin& data, QWidget* parent = nullptr);

    Q_INVOKABLE QVariant get_value(const QString& widget, const QString& property);
    Q_INVOKABLE bool set_value(const QString& widget, const QString& property, const QVariant& value);
};

} // namespace glaxnimate::gui
