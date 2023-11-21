/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TOOLBARSETTINGSWIDGET_H
#define TOOLBARSETTINGSWIDGET_H

#include <memory>
#include <QWidget>

namespace glaxnimate::gui {

class ToolbarSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    ToolbarSettingsWidget(QWidget* parent = nullptr);
    ~ToolbarSettingsWidget();

protected:
    void changeEvent ( QEvent* e ) override;

protected Q_SLOTS:
    void update_preview();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui

#endif // TOOLBARSETTINGSWIDGET_H
