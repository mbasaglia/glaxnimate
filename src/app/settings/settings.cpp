#include "settings.hpp"
#include "app/app_info.hpp"

#include <QSettings>


void app::settings::Settings::load()
{
    QSettings settings(
        AppInfo::instance().writable_data_path("settings.ini"),
        QSettings::IniFormat
    );

    if ( groups.empty() )
        load_metadata();

    for ( const SettingGroup& group : groups )
    {
        QVariantMap values;
        settings.beginGroup(group.slug);
        for ( const Setting& setting : group.settings )
        {
            values[setting.slug] = settings.value(setting.slug, setting.default_value);
            if ( setting.side_effects )
                setting.side_effects(values[setting.slug]);
        }
        settings.endGroup();
        data[group.slug] = values;
    }
}

void app::settings::Settings::save()
{
    QSettings settings(
        AppInfo::instance().writable_data_path("settings.ini"),
        QSettings::IniFormat
    );

    for ( const SettingGroup& group : groups )
    {
        QVariantMap& values = data[group.slug];

        settings.beginGroup(group.slug);
        for ( const Setting& setting : group.settings )
            settings.setValue(setting.slug, setting.get_variant(values));
        settings.endGroup();
    }
}

void app::settings::Settings::add_group ( app::settings::SettingGroup group )
{
    QString slug = group.slug;
    if ( !order.contains(slug) )
    {
        order[slug] = groups.size();
        data[slug] = {};
        groups.push_back(std::move(group));
    }
    else
    {
        groups[order[slug]] = std::move(group);
    }
}

QVariant app::settings::Settings::get_value ( const QString& group, const QString& setting ) const
{
    if ( !order.contains(group) )
        return {};

    return groups[order[group]].get_variant(setting, data[group]);
}

bool app::settings::Settings::set_value ( const QString& group, const QString& setting, const QVariant& value )
{
    if ( !order.contains(group) )
        return false;

    return groups[order[group]].set_variant(setting, data[group], value);
}



