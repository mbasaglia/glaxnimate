#include "settings_group.hpp"

#include <set>

#include "app/settings/widget_builder.hpp"

app::settings::SettingsGroup::SettingsGroup(QString slug, utils::TranslatedString label, const QString& icon, SettingList settings)
    : slug_(std::move(slug)),
        label_(std::move(label)),
        icon_(std::move(icon)),
        settings_(std::move(settings))
{
}

QString app::settings::SettingsGroup::slug() const
{
    return slug_;
}

QString app::settings::SettingsGroup::label() const
{
    return label_;
}

QIcon app::settings::SettingsGroup::icon() const
{
    return QIcon::fromTheme(icon_);
}

void app::settings::SettingsGroup::load(QSettings& settings)
{
    auto avail_keys = settings.childKeys();
    std::set<QString> unprocessed_keys(avail_keys.begin(), avail_keys.end());

    for ( const Setting& setting : settings_ )
    {
        unprocessed_keys.erase(setting.slug);
        values_[setting.slug] = settings.value(setting.slug, setting.default_value);
        if ( setting.side_effects )
            setting.side_effects(values_[setting.slug]);
    }

    for ( const QString& key : unprocessed_keys )
        values_[key] = settings.value(key);
}

void app::settings::SettingsGroup::save(QSettings& settings)
{
    for ( const Setting& setting : settings_ )
        settings.setValue(setting.slug, setting.get_variant(values_));
}

QWidget* app::settings::SettingsGroup::make_widget(QWidget* parent)
{
    return new SettingsGroupWidget(this, parent);
}

bool app::settings::SettingsGroup::has_visible_settings() const
{
    for ( const auto& set : settings_ )
        if ( set.type != Setting::Internal )
            return true;
    return false;
}

QVariant app::settings::SettingsGroup::get_variant(const QString& setting_slug) const
{
    for ( const Setting& setting : settings_ )
        if ( setting.slug == setting_slug )
            return setting.get_variant(values_);
    return {};
}

bool app::settings::SettingsGroup::set_variant(const QString& setting_slug, const QVariant& value)
{
    for ( const Setting& setting : settings_ )
    {
        if ( setting.slug == setting_slug )
        {
            if ( !setting.valid_variant(value) )
                return false;
            values_[setting.slug] = value;
            if ( setting.side_effects )
                setting.side_effects(value);
            return true;
        }
    }
    return false;
}

QVariant app::settings::SettingsGroup::get_default(const QString& setting_slug) const
{
    for ( const Setting& setting : settings_ )
        if ( setting.slug == setting_slug )
            return setting.default_value;
    return {};
}

QVariant app::settings::SettingsGroup::define(const QString& setting_slug, const QVariant& default_value)
{
    for ( const Setting& setting : settings_ )
        if ( setting.slug == setting_slug )
            return setting.get_variant(values_);

    settings_.push_back(Setting{setting_slug, {}, {}, Setting::Internal, default_value});

    auto it = values_.find(setting_slug);
    if ( it != values_.end() )
        return *it;

    return default_value;
}
