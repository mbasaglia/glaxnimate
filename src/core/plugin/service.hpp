#pragma once
#include <QObject>
#include <QSet>
#include <QString>
#include <QAction>
#include <QIcon>

#include "app/settings/setting.hpp"

namespace glaxnimate::plugin {

class PluginScript
{
    Q_GADGET
public:
    QString module;
    QString function;
    app::settings::SettingList settings;

    bool valid() const
    {
        return !module.isEmpty() && !function.isEmpty();
    }
};

enum class ServiceType
{
    Action,
    IoFormat,
};


class Plugin;

class PluginService : public QObject
{
public:
    virtual ~PluginService() = default;

    virtual ServiceType type() const = 0;
    virtual QString name() const = 0;
    virtual void enable() = 0;
    virtual void disable() = 0;
    virtual QIcon service_icon() const = 0;

    Plugin* plugin() const { return owner; }
    void set_plugin(Plugin* plugin) { owner = plugin; }

private:
    Plugin* owner = nullptr;
};

} // namespace glaxnimate::plugin
