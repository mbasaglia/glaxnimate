#include "dock_widget_style.hpp"

#include <QtGlobal>

void style::DockWidgetStyle::drawControl(
    ControlElement element, const QStyleOption* option,
    QPainter* painter, const QWidget* widget) const
{
    if ( widget && element == QStyle::CE_DockWidgetTitle && !widget->windowIcon().isNull() )
    {
        QStyleOptionDockWidget option_copy = *qstyleoption_cast<const QStyleOptionDockWidget *>(option);

        int margin = baseStyle()->pixelMetric(QStyle::PM_DockWidgetTitleMargin);
        QRect title_rect = subElementRect(SE_DockWidgetTitleBarText, option, widget);
        int size = qMin(pixelMetric(QStyle::PM_ToolBarIconSize), title_rect.height());
        int padding = (title_rect.height() - size) / 2;
        QPoint pos(margin + padding + title_rect.left(), title_rect.top() + padding);

#ifndef Q_OS_MACOS
        option_copy.rect = option->rect.adjusted(size+margin*2, 0, 0, 0);
#endif
        QProxyStyle::drawControl(element, &option_copy, painter, widget);

        painter->drawPixmap(pos, widget->windowIcon().pixmap(size, size));

        return;
    }
    QProxyStyle::drawControl(element, option, painter, widget);
}
