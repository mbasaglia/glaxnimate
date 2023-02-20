/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "api_credentials.hpp"

#include <QWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QDesktopServices>

#include "api_keys.hpp"

static QString slugify(const QString& str)
{
    return str.toLower().replace(" ", "_");
}

glaxnimate::gui::settings::ApiCredentials::ApiCredentials()
    : apis_{
        {"Google Fonts", {
            QUrl("https://console.cloud.google.com/apis/credentials"),
            {
                {"Token", "", API_KEY_GOOGLE_FONTS},
                {"URL", "https://www.googleapis.com/webfonts/v1/webfonts"},
            }
        }}
    }
{}

void glaxnimate::gui::settings::ApiCredentials::load ( QSettings & settings )
{
    for ( auto& p : apis_ )
    {
        QString base = slugify(p.first) + "_";
        for ( auto& cred : p.second.credentials )
        {
            QString key = base + slugify(cred.name);
            cred.value = settings.value(key, cred.value).toString();
        }
    }
}

void glaxnimate::gui::settings::ApiCredentials::save ( QSettings & settings )
{
    for ( const auto& p : apis_ )
    {
        QString base = slugify(p.first) + "_";
        for ( const auto& cred : p.second.credentials )
        {
            QString key = base + slugify(cred.name);
            if ( !cred.value.isEmpty() )
                settings.setValue(key, cred.value);
        }
    }
}

QWidget * glaxnimate::gui::settings::ApiCredentials::make_widget ( QWidget * parent )
{
    auto widget = new QWidget(parent);
    auto lay = new QVBoxLayout();
    widget->setLayout(lay);
    for ( auto & api : apis_ )
    {
        auto group = new QGroupBox;
        group->setTitle(api.first);
        auto group_lay = new QFormLayout();
        group->setLayout(group_lay);
        lay->addWidget(group);

        for ( auto& cred : api.second.credentials )
        {
            auto input = new QLineEdit(cred.value);
            QObject::connect(input, &QLineEdit::textChanged, input, [&cred](const QString& text){ cred.value = text; });
            group_lay->addRow(cred.name, input);
        }

        if ( api.second.info_url.isValid() )
        {
            auto link_button = new QPushButton(QIcon::fromTheme("internet-web-browser"), QObject::tr("Information"));
            QObject::connect(link_button, &QPushButton::clicked, link_button, [&api]{
                QDesktopServices::openUrl(api.second.info_url);
            });
            group_lay->addRow(link_button);
        }
    }

    lay->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

    return widget;
}



QVariant glaxnimate::gui::settings::ApiCredentials::get_variant(const QString& setting) const
{
    auto split = setting.split("/");
    return apis_.at(split[0]).credential(split[1]);
}

QString glaxnimate::gui::settings::ApiCredentials::Api::credential(const QString& name) const
{
    for ( const auto& cred : credentials )
    {
        if ( cred.name ==  name )
        {
            if ( cred.value.isEmpty() && !cred.hidden_default.isEmpty() )
                return cred.hidden_default;
            return cred.value;
        }
    }
    return {};
}
