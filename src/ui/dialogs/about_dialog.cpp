#include "about_dialog.hpp"
#include "ui_about_dialog.h"

#include <QSysInfo>
#include <QDesktopServices>

#include "app/application.hpp"

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent), d(new Ui::AboutDialog)
{
    d->setupUi(this);


    d->label_icon->setPixmap(qApp->windowIcon().pixmap(d->label_icon->minimumWidth()));
    d->label_title->setText(qApp->applicationDisplayName());
    d->label_version->setText(qApp->applicationVersion());
    d->line_settings->setText(app::Application::instance()->data_file("settings.ini"));
    d->line_user_data->setText(app::Application::instance()->writable_data_path(""));

    int h = 0;
    int c = 0;
    for ( const QString& str : app::Application::instance()->data_paths_unchecked("") )
    {
        d->view_data->addItem(str);
        h += d->view_data->sizeHintForRow(c++);
    }
    d->view_data->setMinimumHeight(h);

    c = h = 0;
    for ( const QString& str : QIcon::themeSearchPaths() )
    {
        d->view_icons->addItem(str);
        h += d->view_icons->sizeHintForRow(c++);
    }
    d->view_icons->setMinimumHeight(h);


    d->view_system->setItem(0, 0, new QTableWidgetItem(QSysInfo::prettyProductName()));
    d->view_system->setItem(1, 0, new QTableWidgetItem(QT_VERSION_STR));
    d->view_system->setItem(2, 0, new QTableWidgetItem(qVersion()));

}

AboutDialog::~AboutDialog() = default;

void AboutDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->retranslateUi(this);
    }
}

void AboutDialog::open_user_data()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(d->line_user_data->text()));
}

void AboutDialog::open_settings_file()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(d->line_settings->text()));
}

