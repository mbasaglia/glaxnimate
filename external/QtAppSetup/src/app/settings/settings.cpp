/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "settings.hpp"
#include "app/application.hpp"

#include <set>
#include <QSettings>

void app::settings::Settings::load()
{
    QSettings settings = app::Application::instance()->qsettings();

    auto avail_groups = settings.childGroups();
    std::set<QString> unprocessed(avail_groups.begin(), avail_groups.end());
    avail_groups.clear();

    for ( const auto& group : groups_ )
    {
        unprocessed.erase(group->slug());
        settings.beginGroup(group->slug());
        group->load(settings);
        settings.endGroup();
    }
}

void app::settings::Settings::save()
{
    QSettings settings = app::Application::instance()->qsettings();

    for ( const auto& group : groups_ )
    {
        settings.beginGroup(group->slug());
        group->save(settings);
        settings.endGroup();
    }
}

void app::settings::Settings::add_group(QString slug, utils::TranslatedString label, const QString& icon, SettingList settings)
{
    add_group(std::make_unique<SettingsGroup>(std::move(slug), std::move(label), std::move(icon), std::move(settings)));
}

void app::settings::Settings::add_group(CustomSettingsGroup group)
{
    auto slug = group->slug();

    if ( !order.contains(slug) )
        order[slug] = groups_.size();

    groups_.push_back(std::move(group));
}

QVariant app::settings::Settings::get_value ( const QString& group, const QString& setting ) const
{
    if ( !order.contains(group) )
        return {};

    return groups_[order[group]]->get_variant(setting);
}

QVariant app::settings::Settings::get_default(const QString& group, const QString& setting) const
{
    if ( !order.contains(group) )
        return {};

    return groups_[order[group]]->get_default(setting);
}


bool app::settings::Settings::set_value ( const QString& group, const QString& setting, const QVariant& value )
{
    if ( !order.contains(group) )
        return false;

    return groups_[order[group]]->set_variant(setting, value);
}

QVariant app::settings::Settings::define(const QString& group, const QString& setting, const QVariant& default_value)
{
    if ( !order.contains(group) )
        return default_value;

    return groups_[order[group]]->define(setting, default_value);
}
