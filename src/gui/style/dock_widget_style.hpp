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
        QPainter* painter, const QWidget* widget) const override
    {
        if ( widget && element == QStyle::CE_DockWidgetTitle && !widget->windowIcon().isNull() )
        {
            int size = pixelMetric(QStyle::PM_ToolBarIconSize);
            int margin = baseStyle()->pixelMetric(QStyle::PM_DockWidgetTitleMargin);

            QPoint pos(margin + option->rect.left(), margin + option->rect.center().y() - size/2);

            painter->drawPixmap(pos, widget->windowIcon().pixmap(size, size));

            const_cast<QStyleOption*>(option)->rect = option->rect.adjusted(size, 0, 0, 0);
        }
        QProxyStyle::drawControl(element, option, painter, widget);
    }
};

} // namespace style
