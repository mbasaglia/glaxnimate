#include "brush_style.hpp"



QIcon model::BrushStyle::reftarget_icon() const
{
    if ( icon.isNull() )
    {
        icon = QPixmap(32, 32);
        fill_icon(icon);
    }
    return icon;
}

void model::BrushStyle::fill_icon(QPixmap& icon) const
{
    QPainter p(&icon);
    p.fillRect(icon.rect(), brush_style(0));
}
