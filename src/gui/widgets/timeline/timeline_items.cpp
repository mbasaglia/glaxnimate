#include "timeline_items.hpp"


void timeline::LineItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    if ( isSelected() )
        painter->fillRect(option->rect, widget->palette().highlight());
//     else if ( isUnderMouse() )
//         painter->fillRect(option->rect, widget->palette().brush(QPalette::Inactive, QPalette::Highlight));
}
