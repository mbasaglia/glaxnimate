#include "dock_widget_style.hpp"


void style::DockWidgetStyle::drawControl(
    ControlElement element, const QStyleOption* option,
    QPainter* painter, const QWidget* widget) const
{
    if ( widget && element == QStyle::CE_DockWidgetTitle && !widget->windowIcon().isNull() )
    {
        QStyleOptionDockWidget option_copy = *qstyleoption_cast<const QStyleOptionDockWidget *>(option);

        int margin = baseStyle()->pixelMetric(QStyle::PM_DockWidgetTitleMargin);
        QRect title_rect = subElementRect(SE_DockWidgetTitleBarText, option, widget);
        int size = title_rect.height();
        QPoint pos(margin + title_rect.left(), title_rect.top());
        
        option_copy.rect = option->rect.adjusted(size+margin*2, 0, 0, 0);
        QProxyStyle::drawControl(element, &option_copy, painter, widget);

        painter->drawPixmap(pos, widget->windowIcon().pixmap(size, size));

        return;
    }
    QProxyStyle::drawControl(element, option, painter, widget);
}
