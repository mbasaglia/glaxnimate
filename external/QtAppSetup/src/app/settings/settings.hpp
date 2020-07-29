#pragma once

#include <QHash>
#include <QList>
#include <QCoreApplication>

#include "app/settings/setting_group.hpp"
#include "app/settings/custom_settings_group.hpp"


namespace app::settings {

class Settings
{
    Q_DECLARE_TR_FUNCTIONS(Settings)

public:
    using iterator = QList<SettingGroup>::const_iterator;

    static Settings& instance()
    {
        static Settings singleton;
        return singleton;
    }

    QVariant get_value(const QString& group, const QString& setting) const;

    bool set_value(const QString& group, const QString& setting, const QVariant& value);

    void load();

    void save();

    void load_metadata();

    iterator begin() const { return groups.begin(); }
    iterator end() const  { return groups.end(); }

    QVariantMap& group_values(const QString& group)
    {
        return data[group];
    }

    void add_group(SettingGroup group);

    const std::vector<CustomSettingsGroup>& custom_groups() const { return custom_groups_; }
    void add_custom_group(CustomSettingsGroup group) { custom_groups_.push_back(std::move(group)); }

private:
    Settings() = default;
    Settings(const Settings&) = delete;
    ~Settings() = default;

    QList<SettingGroup> groups;
    QHash<QString, int> order;
    QHash<QString, QVariantMap> data;
    std::vector<CustomSettingsGroup> custom_groups_;
};



template<class T>
T get(const QString& group, const QString& setting)
{
    return Settings::instance().get_value(group, setting).value<T>();
}

template<class T>
bool set(const QString& group, const QString& setting, const T& value)
{
    return Settings::instance().set_value(group, setting, QVariant::fromValue(value));
}

} // namespace app::settings
