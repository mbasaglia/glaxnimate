/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <unordered_map>
#include <QUrl>

#include "app/utils/qstring_hash.hpp"
#include "app/settings/custom_settings_group.hpp"


namespace glaxnimate::gui::settings {

class ApiCredentials : public app::settings::CustomSettingsGroupBase
{
public:
    struct Credential
    {
        QString name;
        QString value = "";
        QString hidden_default = "";
    };

    struct Api
    {
        QUrl info_url;
        std::vector<Credential> credentials;

        QString credential(const QString& name) const;
    };

    ApiCredentials();

    QString slug() const override { return "api_credentials"; }
    QIcon icon() const override { return QIcon::fromTheme("dialog-password"); }
    QString label() const override { return QObject::tr("API Credentials"); }
    bool has_visible_settings() const override { return !apis_.empty(); }

    QVariant get_variant(const QString& setting) const override;

    void load ( QSettings & settings ) override;

    void save ( QSettings & settings ) override;

    const Api& api(const QString& name) const
    {
        return apis_.at(name);
    }

    QWidget * make_widget ( QWidget * parent ) override;


private:
    std::map<QString, Api> apis_;
};

} // namespace glaxnimate::gui::settings

