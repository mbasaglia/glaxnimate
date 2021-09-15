#include "io.hpp"
#include "plugin.hpp"

using namespace glaxnimate;

void plugin::IoService::enable()
{
    if ( registered )
        disable();
    registered = io::IoRegistry::instance().register_object(std::make_unique<IoFormat>(this));
}

void plugin::IoService::disable()
{
    if ( registered )
        io::IoRegistry::instance().unregister(registered);
    registered = nullptr;
}

bool plugin::IoFormat::on_open(QIODevice& file, const QString& name, model::Document* document, const QVariantMap& settings)
{
    return service->plugin()->run_script(service->open, {
        PluginRegistry::instance().global_parameter("window"),
        QVariant::fromValue(document),
        QVariant::fromValue(&file),
        name,
        QVariant::fromValue(this),
        settings
    });
}

bool plugin::IoFormat::on_save(QIODevice& file, const QString& name, model::Document* document, const QVariantMap& settings)
{
    return service->plugin()->run_script(service->save, {
        PluginRegistry::instance().global_parameter("window"),
        QVariant::fromValue(document),
        QVariant::fromValue(&file),
        name,
        QVariant::fromValue(this),
        settings
    });
}



std::unique_ptr<app::settings::SettingsGroup> glaxnimate::plugin::IoFormat::open_settings() const
{
    return std::make_unique<app::settings::SettingsGroup>(service->open.settings);
}

std::unique_ptr<app::settings::SettingsGroup> glaxnimate::plugin::IoFormat::save_settings(model::Document*) const
{
    return std::make_unique<app::settings::SettingsGroup>(service->save.settings);
}

