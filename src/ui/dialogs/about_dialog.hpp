#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <memory>
#include <QDialog>

namespace Ui
{
class AboutDialog;
}

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    AboutDialog(QWidget* parent = nullptr);

    ~AboutDialog();

protected:
    void changeEvent(QEvent *e) override;

private slots:
    void open_user_data();
    void open_settings_file();

private:
    std::unique_ptr<Ui::AboutDialog> d;
};

#endif // ABOUTDIALOG_H
