#include "io.hpp"
#include "plugin.hpp"

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
    service->plugin()->run_script(service->open, {
        QVariant::fromValue(document), QVariant::fromValue(&file), name, QVariant::fromValue(this), settings
    });
    return true;
}

bool plugin::IoFormat::on_save(QIODevice& file, const QString& name, model::Document* document, const QVariantMap& settings)
{
    service->plugin()->run_script(service->save, {
        QVariant::fromValue(document), QVariant::fromValue(&file), name, QVariant::fromValue(this), settings
    });
    return true;
}



