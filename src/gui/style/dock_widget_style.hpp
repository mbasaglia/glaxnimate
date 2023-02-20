/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QProxyStyle>
#include <QStyle>
#include <QPainter>
#include <QWidget>
#include <QStyleOption>

namespace glaxnimate::gui::style {

class DockWidgetStyle: public QProxyStyle
{
public:
    void drawControl(ControlElement element, const QStyleOption* option,
        QPainter* painter, const QWidget* widget) const override;
};

} // namespace glaxnimate::gui::style
