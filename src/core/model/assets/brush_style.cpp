#include "brush_style.hpp"

QIcon glaxnimate::model::BrushStyle::instance_icon() const
{
    if ( icon.isNull() )
    {
        icon = QPixmap(32, 32);
        fill_icon(icon);
    }
    return icon;
}

QBrush glaxnimate::model::BrushStyle::constrained_brush_style(FrameTime t, const QRectF& ) const
{
    return brush_style(t);
}
