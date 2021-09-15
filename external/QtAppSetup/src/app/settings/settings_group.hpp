#pragma once

#include <vector>

#include <QIcon>

#include "app/utils/translated_string.hpp"
#include "app/settings/setting.hpp"
#include "custom_settings_group.hpp"

namespace app::settings {

class SettingsGroup : public CustomSettingsGroupBase
{
public:
    using iterator = SettingList::const_iterator;

    SettingsGroup(QString slug, utils::TranslatedString label, const QString& icon, SettingList settings);

    QString slug() const override;

    QString label() const override;

    QIcon icon() const override;

    void load(QSettings& settings) override;

    void save(QSettings& settings) override;

    QWidget* make_widget(QWidget* parent) override;

    bool has_visible_settings() const override;

    QVariant get_variant(const QString& setting_slug) const override;

    bool set_variant(const QString& setting_slug, const QVariant& value) override;

    QVariant get_default(const QString& setting_slug) const override;

    QVariant define(const QString& setting_slug, const QVariant& default_value) override;


    iterator begin() const { return settings_.begin(); }
    iterator end() const  { return settings_.end(); }

    SettingList& settings() { return settings_; }
    QVariantMap& values() { return values_; }

private:
    QString slug_;
    utils::TranslatedString label_;
    QString icon_;
    SettingList settings_;
    QVariantMap values_;
};

} // namespace app::settings

