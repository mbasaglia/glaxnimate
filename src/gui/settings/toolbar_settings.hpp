/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "app/settings/custom_settings_group.hpp"

namespace glaxnimate::gui::settings {

class ToolbarSettingsGroup : public app::settings::CustomSettingsGroupBase
{
public:
    QString slug() const override { return "toolbars"; }
    QIcon icon() const override { return QIcon::fromTheme("configure-toolbars"); }
    QString label() const override { return QObject::tr("Toolbars"); }

    void load ( QSettings & settings ) override;
    void save ( QSettings & settings ) override;

    QWidget * make_widget ( QWidget * parent ) override;

    static Qt::ToolButtonStyle button_style;
    static int icon_size_extent;
    static int tool_icon_size_extent;
    static QSize icon_size() { return QSize(icon_size_extent, icon_size_extent); }
    static QSize tool_icon_size() { return QSize(tool_icon_size_extent, tool_icon_size_extent); }

    static void apply();
};

} // namespace glaxnimate::gui::settings

