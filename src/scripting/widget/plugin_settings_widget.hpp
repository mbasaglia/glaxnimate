#ifndef SCRIPTING_PLUGINSETTINGSWIDGET_H
#define SCRIPTING_PLUGINSETTINGSWIDGET_H

#include <memory>
#include <QWidget>

class QListWidgetItem;

namespace scripting
{

class Plugin;

namespace Ui
{
class PluginSettingsWidget;
}

class PluginSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    PluginSettingsWidget(QWidget* parent = nullptr);

    ~PluginSettingsWidget();


private slots:
    void install_dialog();
    void refresh_plugins();
    void uninstall_current();
    void enable_current();
    void disable_current();
    void current_changed(QListWidgetItem* item);

protected:
    void changeEvent ( QEvent* event ) override;

private:
    void update_entries();
    void clear_selection();

    std::unique_ptr<Ui::PluginSettingsWidget> d;
    Plugin* current = nullptr;
};

}

#endif // SCRIPTING_PLUGINSETTINGSWIDGET_H
