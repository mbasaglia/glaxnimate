#include "named_color.hpp"

GLAXNIMATE_OBJECT_IMPL(model::NamedColor)

QString model::NamedColor::type_name_human() const
{
    return tr("Unnamed Color");
}

QBrush model::NamedColor::brush_style(FrameTime t) const
{
    return color.get_at(t);
}

void model::NamedColor::fill_icon(QPixmap& icon) const
{
    icon.fill(color.get_at(0));
}
