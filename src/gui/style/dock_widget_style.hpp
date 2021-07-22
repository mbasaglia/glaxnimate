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
