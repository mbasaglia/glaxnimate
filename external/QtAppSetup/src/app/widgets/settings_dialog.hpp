#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <memory>
#include <QDialog>


class QAbstractButton;

namespace app {
namespace Ui { class SettingsDialog; }

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

} // namespace app
#endif // SETTINGSDIALOG_H
