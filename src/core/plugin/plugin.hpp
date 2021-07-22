#pragma once

#include <memory>

#include <QDir>

#include "app/log/log.hpp"
#include "plugin/service.hpp"

namespace app::scripting {

class ScriptEngine;

} // namespace app::scripting

namespace glaxnimate::plugin {

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
    QString description;
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
                return QIcon(data_.dir.absoluteFilePath(icon));
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

    bool run_script(const PluginScript& script, const QVariantList& args) const;

    bool enabled() const { return enabled_; }

    app::log::Log logger() const
    {
        return {"Plugins", data_.name};
    }

private:
    PluginData data_;
    bool enabled_ = false;
    bool user_installed_ = false;
    QIcon icon_;
};

class Executor;


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

    Executor* executor() const;
    void set_executor(Executor* exec);
    QVariant global_parameter(const QString& name) const;

signals:
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
    Executor* executor_ = nullptr;
    QMap<QString, int> names;
    app::log::Log logger{"Plugins"};
};

} // namespace glaxnimate::plugin
