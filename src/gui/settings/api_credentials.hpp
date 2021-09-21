#pragma once


#include <QUrl>

#include "app/settings/custom_settings_group.hpp"


namespace glaxnimate::gui::settings {

class ApiCredentials : public app::settings::CustomSettingsGroupBase
{
public:
    struct Credential
    {
        QString name;
        QString value = "";
    };

    struct Api
    {
        QUrl info_url;
        std::vector<Credential> credentials;

        QString credential(const QString& name) const
        {
            for ( const auto& cred : credentials )
                if ( cred.name ==  name )
                    return cred.value;
            return {};
        }
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

