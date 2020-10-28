#pragma once

#include <memory>

#include <QString>
#include <QIcon>
#include <QDir>
#include <QAction>
#include <QSet>

#include "app/settings/setting.hpp"
#include "app/log/log.hpp"


namespace app::scripting {

class ScriptEngine;

} // namespace app::scripting

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
    Import,
    Export,
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


struct PluginData
{
    QDir dir;
    QString id;

    int version = 0;
    const app::scripting::ScriptEngine* engine = nullptr;
    QString engine_name;
    QString name;
    QString author;
    QString icon;
    std::vector<std::unique_ptr<PluginService>> services;
};

class Plugin
{
public:
    Plugin(PluginData data, bool user_installed)
        : data_(std::move(data)), user_installed_(user_installed)
        {
            icon_ = QIcon::fromTheme("libreoffice-extension");
            icon_ = make_icon(data_.icon);
            for ( const auto& ps : data_.services )
                ps->set_plugin(this);
        }

    const PluginData& data() const { return data_; }

    QString file(const QString& path) const
    {
        if ( !QDir::isRelativePath(path) )
            return {};

        if ( !data_.dir.exists(path) )
            return {};

        return data_.dir.cleanPath(path);
    }

    QIcon make_icon(const QString& icon) const
    {
        if ( !icon.isEmpty() )
        {
            if ( icon.startsWith("theme:") )
                return QIcon::fromTheme(icon.mid(6));
            if ( data_.dir.exists(icon) )
                return QIcon(data_.dir.cleanPath(icon));
        }
        return icon_;
    }

    bool available() const
    {
        return data_.engine;
    }

    bool can_enable() const
    {
        return !enabled_ && data_.engine;
    }

    bool can_disable() const
    {
        return enabled_;
    }

    void enable()
    {
        if ( !can_enable() )
            return;

        for ( const auto& svc : data_.services )
            svc->enable();

        enabled_ = true;
    }

    void disable()
    {
        if ( !can_disable() )
            return;

        for ( const auto& svc : data_.services )
            svc->disable();

        enabled_ = false;
    }

    bool user_installed() const
    {
        return user_installed_;
    }

    const QIcon& icon() const
    {
        return icon_;
    }

    void run_script(const PluginScript& script, const QVariantMap& settings) const;

    bool enabled() const { return enabled_; }

private:
    PluginData data_;
    bool enabled_ = false;
    bool user_installed_ = false;
    QIcon icon_;
};


class PluginRegistry : public QObject
{
    Q_OBJECT

public:
    static PluginRegistry& instance()
    {
        static PluginRegistry instance;
        return instance;
    }

    void load();

    const std::vector<std::unique_ptr<Plugin>>& plugins() const { return plugins_; }

    bool load_plugin(const QString& path, bool user_installed);

    Plugin* plugin(const QString& id) const;

signals:
    void script_needs_running(const Plugin& plugin, const PluginScript& script, const QVariantMap& settings);
    void loaded();

private:
    PluginRegistry() = default;
    PluginRegistry(const PluginRegistry&) = delete;
    ~PluginRegistry() = default;

    void load_service(const QJsonObject& jobj, PluginData& data) const;
    PluginScript load_script(const QJsonObject& jobj) const;
    void load_setting(const QJsonObject& jobj, PluginScript& script) const;
    QVariantMap load_choices(const QJsonValue& val) const;

    std::vector<std::unique_ptr<Plugin>> plugins_;
    QMap<QString, int> names;
    app::log::Log logger{"Plugins"};
};

} // namespace plugin
