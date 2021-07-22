#pragma once

#include "service.hpp"

namespace glaxnimate::plugin {


class ActionService;

class PluginActionRegistry : public QObject
{
    Q_OBJECT

public:
    static PluginActionRegistry& instance()
    {
        static PluginActionRegistry instance;
        return instance;
    }

    QAction* make_qaction(ActionService* action);

    void add_action(ActionService* action);
    void remove_action(ActionService* action);

    const std::vector<ActionService*>& enabled() const;

signals:
    void action_added(ActionService* action, ActionService* sibling_before);
    void action_removed(ActionService*);

private:
    std::vector<ActionService*>::iterator find(ActionService* as);
    static bool compare(ActionService* a, ActionService* b);

    PluginActionRegistry() = default;
    ~PluginActionRegistry() = default;
    std::vector<ActionService*> enabled_actions;
};

class ActionService : public PluginService
{
    Q_OBJECT

public:
    ServiceType type() const override { return ServiceType::Action; }
    QString name() const override { return label; }
    void enable() override { PluginActionRegistry::instance().add_action(this); }
    void disable() override
    {
        PluginActionRegistry::instance().remove_action(this);
        emit disabled();
    }
    QIcon service_icon() const override;

    QString label;
    QString tooltip;
    QString icon;
    PluginScript script;

public slots:
    void trigger() const;

signals:
    void disabled();
};


} // namespace glaxnimate::plugin
