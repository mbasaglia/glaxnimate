#include "about_dialog.hpp"
#include "ui_about_dialog.h"

#include <QSysInfo>
#include <QDesktopServices>
#include <QMessageBox>
#include <QClipboard>
#include <QUrl>

#include "glaxnimate_app.hpp"
#include "utils/trace.hpp"
#include "utils/gzip.hpp"

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent), d(new Ui::AboutDialog)
{
    d->setupUi(this);


    d->label_icon->setPixmap(qApp->windowIcon().pixmap(d->label_icon->minimumWidth()));
    d->label_title->setText(qApp->applicationDisplayName());
    d->label_version->setText(qApp->applicationVersion());
    d->line_settings->setText(app::Application::instance()->data_file("settings.ini"));
    d->line_user_data->setText(app::Application::instance()->writable_data_path(""));
    d->line_backup->setText(GlaxnimateApp::instance()->backup_path());

    populate_view(d->view_data, app::Application::instance()->data_paths_unchecked(""));
    populate_view(d->view_icons, QIcon::themeSearchPaths());


    int row = 0;
    d->view_system->verticalHeaderItem(0)->setText(qApp->applicationDisplayName());
    d->view_system->setItem(row++, 0, new QTableWidgetItem(qApp->applicationVersion()));
    d->view_system->setItem(row++, 0, new QTableWidgetItem(QSysInfo::prettyProductName()));
    d->view_system->setItem(row++, 0, new QTableWidgetItem(QSysInfo::kernelType() + " " + QSysInfo::kernelVersion()));
    d->view_system->setItem(row++, 0, new QTableWidgetItem(QSysInfo::currentCpuArchitecture()));
    d->view_system->setItem(row++, 0, new QTableWidgetItem(QT_VERSION_STR));
    d->view_system->setItem(row++, 0, new QTableWidgetItem(qVersion()));
    d->view_system->setItem(row++, 0, new QTableWidgetItem(utils::gzip::zlib_version()));
    d->view_system->setItem(row++, 0, new QTableWidgetItem(utils::trace::Tracer::potrace_version()));

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

void AboutDialog::populate_view(QListWidget* wid, const QStringList& paths)
{
    int h = 0;
    int c = 0;
    for ( const QString& str : paths )
    {
        if ( str.startsWith(":/") )
            continue;
        wid->addItem(str);
        h += wid->sizeHintForRow(c++);
    }
    h += h/c/2;
    wid->setMinimumHeight(h);
}

void AboutDialog::about_qt()
{
    QMessageBox::aboutQt(this);
}

void AboutDialog::copy_system()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QString text_template("%1: %2\n");

    QString text;
    for ( int i = 0; i < d->view_system->rowCount(); i++ )
        text += text_template.arg(d->view_system->verticalHeaderItem(i)->text()).arg(d->view_system->item(i, 0)->text());

    clipboard->setText(text);
}


void AboutDialog::dir_open(const QModelIndex& index)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(index.data().toString()));
}

void AboutDialog::open_backup()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(d->line_backup->text()));
}
