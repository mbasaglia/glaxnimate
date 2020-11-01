#include "action.hpp"
#include "plugin.hpp"

#include "app/settings/widget_builder.hpp"

QAction * plugin::PluginActionRegistry::make_qaction ( plugin::ActionService* action )
{
    QAction* act = new QAction;
    act->setIcon(action->plugin()->make_icon(action->icon));
    if ( action->label.isEmpty() )
        act->setText(action->plugin()->data().name);
    else
        act->setText(action->label);
    act->setToolTip(action->tooltip);
    connect(act, &QAction::triggered, action, &ActionService::trigger);
    connect(action, &ActionService::disabled, act, &QAction::deleteLater);

    return act;
}

void plugin::PluginActionRegistry::add_action ( plugin::ActionService* action )
{
    if ( enabled_actions.contains(action) )
        return;

    enabled_actions.insert(action);
    emit action_added(action);
}

void plugin::PluginActionRegistry::remove_action ( plugin::ActionService* action )
{
    if ( !enabled_actions.contains(action) )
        return;

    enabled_actions.remove(action);
    emit action_removed(action);

}

QIcon plugin::ActionService::service_icon() const
{
    return plugin()->make_icon(icon);
}


void plugin::ActionService::trigger() const
{
    QVariantMap settings_value;
    if ( !script.settings.empty() )
    {
        if ( !app::settings::WidgetBuilder().show_dialog(
            script.settings, settings_value, plugin()->data().name
        ) )
            return;
    }

    plugin()->run_script(script, {
        PluginRegistry::instance().global_parameter("window"),
        PluginRegistry::instance().global_parameter("document"),
        settings_value
    });
}

