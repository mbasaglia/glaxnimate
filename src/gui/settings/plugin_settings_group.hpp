/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "app/settings/custom_settings_group.hpp"
#include "widgets/settings/plugin_settings_widget.hpp"
#include "plugin/plugin.hpp"

namespace glaxnimate::gui::settings {

class PluginSettingsGroup : public app::settings::CustomSettingsGroupBase
{
public:
    PluginSettingsGroup(QStringList default_enabled)
    : enabled(std::move(default_enabled)) {}

    QString slug() const override { return "plugins"; }
    QIcon icon() const override { return QIcon::fromTheme("system-software-install"); }
    QString label() const override { return QObject::tr("Plugins"); }
    void load ( QSettings & settings ) override
    {
        plugin::PluginRegistry::instance().load();

        enabled = settings.value("enabled", enabled).toStringList();

        for ( const auto& plugin : plugin::PluginRegistry::instance().plugins() )
            if ( enabled.contains(plugin->data().id) )
                plugin->enable();
    }

    void save ( QSettings & settings ) override
    {
        enabled.clear();

        for ( const auto& plugin : plugin::PluginRegistry::instance().plugins() )
            if ( plugin->enabled() )
                enabled.push_back(plugin->data().id);

        settings.setValue("enabled", enabled);
    }

    QWidget * make_widget ( QWidget * parent ) override { return new PluginSettingsWidget(parent); }

private:
    QStringList enabled;
};

} // namespace glaxnimate::gui::settings
