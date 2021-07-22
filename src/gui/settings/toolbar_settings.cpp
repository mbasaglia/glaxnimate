#include "toolbar_settings.hpp"

#include <QApplication>

#include "widgets/settings/toolbar_settings_widget.hpp"

#include <QMainWindow>
#include <QToolBar>

using namespace glaxnimate::gui;

Qt::ToolButtonStyle settings::ToolbarSettingsGroup::button_style = Qt::ToolButtonIconOnly;
int settings::ToolbarSettingsGroup::icon_size_extent = 22;
int settings::ToolbarSettingsGroup::tool_icon_size_extent = 32;

QWidget * settings::ToolbarSettingsGroup::make_widget ( QWidget * parent )
{
    return new ToolbarSettingsWidget(parent);
}

void settings::ToolbarSettingsGroup::load(QSettings& settings)
{
    icon_size_extent = settings.value("icon_size", icon_size_extent).toInt();
    tool_icon_size_extent = settings.value("tool_icon_size", tool_icon_size_extent).toInt();
    button_style = Qt::ToolButtonStyle(settings.value("button_style", int(button_style)).toInt());
}

void settings::ToolbarSettingsGroup::save(QSettings& settings)
{
    settings.setValue("icon_size", icon_size_extent);
    settings.setValue("tool_icon_size", tool_icon_size_extent);
    settings.setValue("button_style", int(button_style));
}

void settings::ToolbarSettingsGroup::apply()
{
    for ( QWidget *widget : QApplication::topLevelWidgets() )
    {
        if ( auto window = qobject_cast<QMainWindow*>(widget) )
        {
            window->setIconSize(icon_size());
            window->setToolButtonStyle(button_style);
            if ( auto tb_tools = window->findChild<QToolBar*>("toolbar_tools") )
            {
                tb_tools->setIconSize(tool_icon_size());
            }
        }
    }
}
