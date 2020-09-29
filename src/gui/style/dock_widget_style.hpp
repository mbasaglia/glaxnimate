#pragma once

#include <QProxyStyle>
#include <QStyle>
#include <QPainter>
#include <QWidget>
#include <QStyleOption>

namespace style {

class DockWidgetStyle: public QProxyStyle
{
public:
    void drawControl(ControlElement element, const QStyleOption* option,
        QPainter* painter, const QWidget* widget) const override;
};

} // namespace style
