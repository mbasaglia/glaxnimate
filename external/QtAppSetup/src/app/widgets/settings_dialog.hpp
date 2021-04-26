#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <memory>
#include <QDialog>


namespace app {

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(QWidget* parent=nullptr);
    ~SettingsDialog();

protected:
    void changeEvent(QEvent *e) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace app
#endif // SETTINGSDIALOG_H
