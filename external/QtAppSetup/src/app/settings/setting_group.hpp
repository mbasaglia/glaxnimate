#pragma once

#include <vector>

#include <QIcon>

#include "app/settings/setting.hpp"

namespace app::settings {

class SettingGroup
{
public:
    using iterator = SettingList::const_iterator;

    QString slug;
    QString label;
    QString icon;
    SettingList settings;

    bool has_visible_settings() const
    {
        for ( const auto& set : settings )
            if ( set.type != Setting::Internal )
                return true;
        return false;
    }

    QVariant get_variant(const QString& setting_slug, const QVariantMap& values) const
    {
        for ( const Setting& setting : settings )
            if ( setting.slug == setting_slug )
                return setting.get_variant(values);
        return {};
    }

    bool set_variant(const QString& setting_slug, QVariantMap& values, const QVariant& value) const
    {
        for ( const Setting& setting : settings )
        {
            if ( setting.slug == setting_slug )
            {
                if ( !setting.valid_variant(value) )
                    return false;
                values[setting.slug] = value;
                if ( setting.side_effects )
                    setting.side_effects(value);
                return true;
            }
        }
        return false;
    }

    QVariant get_default(const QString& setting_slug) const
    {
        for ( const Setting& setting : settings )
            if ( setting.slug == setting_slug )
                return setting.default_value;
        return {};
    }

    QVariant define(const QString& setting_slug, QVariantMap& values, const QVariant& default_value)
    {
        for ( const Setting& setting : settings )
            if ( setting.slug == setting_slug )
                return setting.get_variant(values);

        settings.push_back(Setting{setting_slug, {}, {}, Setting::Internal, default_value});
        return default_value;
    }


    iterator begin() const { return settings.begin(); }
    iterator end() const  { return settings.end(); }
};

} // namespace app::settings

