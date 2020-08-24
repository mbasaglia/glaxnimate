#include "io_status_dialog.hpp"
#include "ui_io_status_dialog.h"


class IoStatusDialog::Private : public Ui::IoStatusDialog
{
public:
    bool delete_on_close;
    QIcon icon;
    bool has_errors = false;
    bool finished = false;
    io::ImportExport* ie = nullptr;
};

IoStatusDialog::IoStatusDialog(const QIcon& icon, const QString& title, bool delete_on_close, QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->delete_on_close = delete_on_close;
    d->icon = icon;
    d->setupUi(this);
    setWindowTitle(title);
}

IoStatusDialog::~IoStatusDialog() = default;


void IoStatusDialog::_on_completed(bool success)
{
    d->finished = true;

    if ( success && !d->has_errors )
        hide();
    else
        show();

    d->progress_bar->hide();
    d->button_box->setEnabled(true);

    if ( !success )
    {
        d->icon_label->setPixmap(QIcon::fromTheme("dialog-error").pixmap(64));
        d->group_box->show();
        d->list_widget->addItem(new QListWidgetItem(QIcon::fromTheme("data-error"), tr("Operation Failed")));
    }
    else
    {
        d->icon_label->setPixmap(QIcon::fromTheme("dialog-warning").pixmap(64));
    }

    if ( d->delete_on_close && !isVisible() )
        deleteLater();

    disconnect(d->ie, nullptr, this, nullptr);
    d->ie = nullptr;
}

void IoStatusDialog::_on_error(const QString& message)
{
    d->group_box->show();
    d->list_widget->addItem(new QListWidgetItem(QIcon::fromTheme("data-warning"), message));
    d->has_errors = true;
    show();

}

void IoStatusDialog::_on_progress(int value)
{
    d->progress_bar->setValue(value);
}

void IoStatusDialog::_on_progress_max_changed(int max)
{
    d->progress_bar->setMaximum(max);
    show();
}

void IoStatusDialog::reset(io::ImportExport* ie, const QString& label)
{
    d->progress_bar->show();
    d->progress_bar->setValue(0);
    d->progress_bar->setMaximum(0);
    d->list_widget->clear();
    d->button_box->setEnabled(false);
    d->group_box->hide();
    d->label->setText(label);
    d->finished = false;
    d->icon_label->setPixmap(d->icon.pixmap(64));
    d->has_errors = false;


    d->ie = ie;
    connect(ie, &io::ImportExport::error, this, &IoStatusDialog::_on_error);
    connect(ie, &io::ImportExport::progress, this, &IoStatusDialog::_on_progress);
    connect(ie, &io::ImportExport::progress_max_changed, this, &IoStatusDialog::_on_progress_max_changed);
    connect(ie, &io::ImportExport::completed, this, &IoStatusDialog::_on_completed);
}

void IoStatusDialog::closeEvent(QCloseEvent* ev)
{
    if ( d->finished && d->delete_on_close )
        deleteLater();

    QDialog::closeEvent(ev);
}



void IoStatusDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->retranslateUi(this);
    }
}
