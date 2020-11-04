#pragma once

#include "io/io_registry.hpp"

#include "service.hpp"

namespace plugin {

class IoService : public PluginService
{
public:
    ServiceType type() const override { return ServiceType::IoFormat; }
    QString name() const override { return label; }
    void enable() override;
    void disable() override;

    QIcon service_icon() const override { return QIcon::fromTheme("document-save"); }

    QString label;
    QStringList extensions;
    PluginScript open;
    PluginScript save;
    bool auto_open;

    io::ImportExport* registered = nullptr;
};


class IoFormat : public io::ImportExport
{
    Q_OBJECT
public:
    IoFormat(IoService* service) : service(service) {}

    QString name() const override { return service->label; }
    QStringList extensions() const override { return service->extensions; }
    bool can_save() const override { return service->save.valid(); }
    bool can_open() const override { return service->open.valid(); }
    io::SettingList open_settings() const override { return service->open.settings; }
    io::SettingList save_settings() const override { return service->save.settings; }

protected:
    bool auto_open() const override { return service->auto_open; }
    bool on_save(QIODevice& file, const QString&, model::Document* document, const QVariantMap&) override;
    bool on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&) override;

private:
    IoService* service;
};



} // namespace plugin
