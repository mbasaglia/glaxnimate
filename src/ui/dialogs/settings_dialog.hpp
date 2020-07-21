#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <memory>
#include <QDialog>

namespace Ui
{
class SettingsDialog;
}

class QAbstractButton;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(QWidget* parent=nullptr);
    ~SettingsDialog();

protected:
    void changeEvent(QEvent *e) override;

private:
    std::unique_ptr<Ui::SettingsDialog> d;
};

#endif // SETTINGSDIALOG_H
