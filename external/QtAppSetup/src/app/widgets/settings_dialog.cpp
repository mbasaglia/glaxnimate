#include "settings_dialog.hpp"
#include "ui_settings_dialog.h"

#include <QFormLayout>

#include "app/settings/settings.hpp"
#include "app/settings/widget_builder.hpp"


app::SettingsDialog::SettingsDialog ( QWidget* parent ) :
    QDialog(parent), d(std::make_unique<app::Ui::SettingsDialog>())
{
    d->setupUi(this);

    app::settings::WidgetBuilder bob;
    app::settings::Settings::instance().load_metadata();
    for ( const auto& group : app::settings::Settings::instance() )
    {
        if ( !group.has_visible_settings() )
            continue;

        new QListWidgetItem(QIcon::fromTheme(group.icon), group.label, d->list_widget);
        QWidget* page = new QWidget();
        d->stacked_widget->addWidget(page);
        QFormLayout* lay = new QFormLayout(page);
        page->setLayout(lay);

        QVariantMap& target = app::settings::Settings::instance().group_values(group.slug);
        bob.add_widgets(group.settings, page, lay, target, group.slug + "__");
    }

    for ( const auto& group : app::settings::Settings::instance().custom_groups() )
    {
        new QListWidgetItem(group->icon(), group->label(), d->list_widget);
        d->stacked_widget->addWidget(group->make_widget(d->stacked_widget));
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
        app::settings::WidgetBuilder bob;
        app::settings::Settings::instance().load_metadata();
        int i = 0;
        for ( const auto& group : app::settings::Settings::instance() )
        {
            if ( !group.has_visible_settings() )
                continue;
            bob.translate_widgets(group.settings, this, group.slug + "__");
            d->list_widget->item(i)->setText(group.label);
            i++;
        }
    }
}
