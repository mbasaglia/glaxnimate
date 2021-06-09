#ifndef TOOLBARSETTINGSWIDGET_H
#define TOOLBARSETTINGSWIDGET_H

#include <memory>
#include <QWidget>

class ToolbarSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    ToolbarSettingsWidget(QWidget* parent = nullptr);
    ~ToolbarSettingsWidget();

protected:
    void changeEvent ( QEvent* e ) override;

protected slots:
    void update_preview();

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // TOOLBARSETTINGSWIDGET_H
