/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "settings_dialog.hpp"
#include "ui_settings_dialog.h"

#include <QFormLayout>

#include "app/application.hpp"
#include "app/settings/settings.hpp"
#include "app/widgets/no_close_on_enter.hpp"


class app::SettingsDialog::Private : public app::Ui::SettingsDialog
{
public:
    app::widgets::NoCloseOnEnter ncoe;
};


static QIcon best_icon(const QIcon& icon, const QSize& target_size)
{
    for ( const auto& size : icon.availableSizes() )
        if ( size.width() >= target_size.width() )
            return icon;
    return icon.pixmap(target_size);;
}


app::SettingsDialog::SettingsDialog ( QWidget* parent ) :
    QDialog(parent), d(std::make_unique<Private>())
{
    d->setupUi(this);
    installEventFilter(&d->ncoe);

    for ( const auto& group : app::settings::Settings::instance().groups() )
    {
        if ( group->has_visible_settings() )
        {
            new QListWidgetItem(best_icon(group->icon(), d->list_widget->iconSize()), group->label(), d->list_widget);
            d->stacked_widget->addWidget(group->make_widget(d->stacked_widget));
        }
    }

    d->list_widget->setCurrentRow(0);
}

app::SettingsDialog::~SettingsDialog() = default;


void app::SettingsDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->retranslateUi(this);
        int i = 0;
        for ( const auto& group : app::settings::Settings::instance() )
        {
            if ( group->has_visible_settings() )
            {
                d->list_widget->item(i)->setText(group->label());
                i++;
            }
        }
    }
}
