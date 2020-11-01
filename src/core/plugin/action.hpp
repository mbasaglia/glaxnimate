#pragma once

#include "service.hpp"

namespace plugin {


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

    const QSet<ActionService*>& enabled() const { return enabled_actions; }

signals:
    void action_added(ActionService*);
    void action_removed(ActionService*);

private:
    PluginActionRegistry() = default;
    ~PluginActionRegistry() = default;
    QSet<ActionService*> enabled_actions;
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


} // namespace plugin
