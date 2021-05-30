#include "settings.hpp"
#include "app/application.hpp"

#include <set>
#include <QSettings>

void app::settings::Settings::load()
{
    QSettings settings = app::Application::instance()->qsettings();

    if ( groups.empty() )
        load_metadata();

    auto avail_groups = settings.childGroups();
    std::set<QString> unprocessed(avail_groups.begin(), avail_groups.end());
    avail_groups.clear();

    for ( const SettingGroup& group : groups )
    {
        QVariantMap values;
        settings.beginGroup(group.slug);

        auto avail_keys = settings.childKeys();
        std::set<QString> unprocessed_keys(avail_keys.begin(), avail_keys.end());

        for ( const Setting& setting : group.settings )
        {
            unprocessed_keys.erase(setting.slug);
            values[setting.slug] = settings.value(setting.slug, setting.default_value);
            if ( setting.side_effects )
                setting.side_effects(values[setting.slug]);
        }

        for ( const QString& key : unprocessed_keys )
            values[key] = settings.value(key);

        settings.endGroup();
        data[group.slug] = values;
        unprocessed.erase(group.slug);
    }

    for ( const auto& group : custom_groups_ )
    {
        unprocessed.erase(group->slug());
        settings.beginGroup(group->slug());
        group->load(settings);
        settings.endGroup();
    }

    for ( const QString& slug : unprocessed )
    {
        QVariantMap values;
        settings.beginGroup(slug);
        for ( const QString&  key : settings.childKeys() )
            values[key] = settings.value(key);
        settings.endGroup();
        data[slug] = values;
    }
}

void app::settings::Settings::save()
{
    QSettings settings = app::Application::instance()->qsettings();

    for ( const SettingGroup& group : groups )
    {
        QVariantMap& values = data[group.slug];

        settings.beginGroup(group.slug);
        for ( const Setting& setting : group.settings )
            settings.setValue(setting.slug, setting.get_variant(values));
        settings.endGroup();
    }

    for ( const auto& group : custom_groups_ )
    {
        settings.beginGroup(group->slug());
        group->save(settings);
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

QVariant app::settings::Settings::get_default(const QString& group, const QString& setting) const
{
    if ( !order.contains(group) )
        return {};

    return groups[order[group]].get_default(setting);
}


bool app::settings::Settings::set_value ( const QString& group, const QString& setting, const QVariant& value )
{
    if ( !order.contains(group) )
        return false;

    return groups[order[group]].set_variant(setting, data[group], value);
}

void app::settings::Settings::load_metadata()
{
    app::Application::instance()->load_settings_metadata();
}

QVariant app::settings::Settings::define(const QString& group, const QString& setting, const QVariant& default_value)
{
    if ( !order.contains(group) )
        return default_value;

    return groups[order[group]].define(setting, data[group], default_value);
}
