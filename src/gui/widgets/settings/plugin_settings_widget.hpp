/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SCRIPTING_PLUGINSETTINGSWIDGET_H
#define SCRIPTING_PLUGINSETTINGSWIDGET_H

#include <memory>
#include <QWidget>

class QListWidgetItem;

namespace glaxnimate::plugin
{
class Plugin;
}

namespace glaxnimate::gui {

namespace Ui
{
class PluginSettingsWidget;
}

class PluginSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    PluginSettingsWidget(QWidget* parent = nullptr);

    ~PluginSettingsWidget();


private slots:
    void install_dialog();
    void refresh_plugins();
    void uninstall_current();
    void enable_current();
    void disable_current();
    void current_changed(QListWidgetItem* item);

protected:
    void changeEvent ( QEvent* event ) override;

private:
    void update_entries();
    void clear_selection();

    std::unique_ptr<Ui::PluginSettingsWidget> d;
    plugin::Plugin* current = nullptr;
};

} // namespace glaxnimate::gui
#endif // SCRIPTING_PLUGINSETTINGSWIDGET_H
