#include "settings_dialog.hpp"
#include "ui_settings_dialog.h"

#include <QFormLayout>

#include "app/settings/settings.hpp"
#include "app/settings/widget_builder.hpp"


SettingsDialog::SettingsDialog ( QWidget* parent ) :
    QDialog(parent), d(std::make_unique<Ui::SettingsDialog>())
{
    d->setupUi(this);

    app::settings::WidgetBuilder bob;
    app::settings::Settings::instance().load_metadata();
    for ( const auto& group : app::settings::Settings::instance() )
    {
        new QListWidgetItem(QIcon::fromTheme(group.icon), group.label, d->list_widget);
        QWidget* page = new QWidget();
        d->stacked_widget->addWidget(page);
        QFormLayout* lay = new QFormLayout(page);
        page->setLayout(lay);
        bob.add_widgets(group.settings, page, lay, app::settings::Settings::instance().group_values(group.slug));
    }

    d->list_widget->setCurrentRow(0);
}

SettingsDialog::~SettingsDialog() = default;



