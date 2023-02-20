/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QTabBar>

namespace glaxnimate::gui {

class ClickableTabBar : public QTabBar
{
    Q_OBJECT

public:
    using QTabBar::QTabBar;

protected:
    void mouseReleaseEvent(QMouseEvent * event) override;

signals:
    void context_menu_requested(int index);
};

} // namespace glaxnimate::gui
