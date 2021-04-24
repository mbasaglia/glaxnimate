#pragma once

#include <QHash>
#include <QList>
#include <QCoreApplication>

#include "app/settings/setting_group.hpp"
#include "app/settings/custom_settings_group.hpp"


namespace app::settings {

/**
 * \brief Singleton for handling settings
 */
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

    /**
     * \brief Get the value of the given setting, or add a new (internal) setting if it hasn't been declared
     */
    QVariant define(const QString& group, const QString& setting, const QVariant& default_value);

    /**
     * \brief Get the value of a declared setting
     */
    QVariant get_value(const QString& group, const QString& setting) const;

    /**
     * \brief Set the value of a declared setting
     */
    bool set_value(const QString& group, const QString& setting, const QVariant& value);

    /**
     * \brief Get the default value of a declared setting
     */
    QVariant get_default(const QString& group, const QString& setting) const;

    /**
     * \brief Load all settings
     */
    void load();

    /**
     * \brief Save all settings
     */
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



/**
 * \brief Get the value of a declared setting
 */
template<class T>
T get(const QString& group, const QString& setting)
{
    return Settings::instance().get_value(group, setting).value<T>();
}

/**
 * \brief Get the value of a declared setting
 * \returns \p default_value if the setting isn't found
 */
template<class T>
T get(const QString& group, const QString& setting, const T& defval)
{
    auto var = Settings::instance().get_value(group, setting);
    if ( var.canConvert<T>() )
        return var.value<T>();
    return defval;
}

/**
 * \brief Get the default value of a declared setting
 */
template<class T>
T get_default(const QString& group, const QString& setting)
{
    return Settings::instance().get_default(group, setting).value<T>();
}

/**
 * \brief Set the value of a declared setting
 */
template<class T>
bool set(const QString& group, const QString& setting, const T& value)
{
    return Settings::instance().set_value(group, setting, QVariant::fromValue(value));
}


/**
 * \brief Get the value of the given setting, or add a new (internal) setting if it hasn't been declared
 */
template<class T>
T define(const QString& group, const QString& setting, const T& default_value)
{
    QVariant var = Settings::instance().define(group, setting, QVariant::fromValue(default_value));
    if ( var.canConvert<T>() )
        return var.value<T>();
    return default_value;
}

} // namespace app::settings
