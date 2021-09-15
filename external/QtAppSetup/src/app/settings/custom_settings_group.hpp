#pragma once

#include <memory>
#include <QSettings>
#include <QIcon>

namespace app::settings {

class CustomSettingsGroupBase
{
public:
    virtual ~CustomSettingsGroupBase() = default;
    virtual QString slug() const = 0;
    virtual QString label() const = 0;
    virtual QIcon icon() const = 0;
    virtual void load(QSettings& settings) = 0;
    virtual void save(QSettings& settings) = 0;
    virtual QWidget* make_widget(QWidget* parent) = 0;
    virtual bool has_visible_settings() const { return true; }
    virtual QVariant get_variant(const QString& setting) const
    {
        Q_UNUSED(setting);
        return {};

    }
    virtual bool set_variant(const QString& setting_slug, const QVariant& value)
    {
        Q_UNUSED(setting_slug);
        Q_UNUSED(value);
        return false;
    }

    virtual QVariant get_default(const QString& setting_slug) const
    {
        Q_UNUSED(setting_slug);
        return {};
    }
    virtual QVariant define(const QString& setting_slug, const QVariant& default_value)
    {
        Q_UNUSED(setting_slug);
        Q_UNUSED(default_value);
        return {};
    }
};


using CustomSettingsGroup = std::unique_ptr<CustomSettingsGroupBase>;

} // namespace app::settings
