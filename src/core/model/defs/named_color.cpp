#include "named_color.hpp"

#include "model/document.hpp"
#include "model/defs/defs.hpp"
#include "command/object_list_commands.hpp"

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

bool model::NamedColor::remove_if_unused(bool clean_lists)
{
    if ( clean_lists && users().empty() )
    {
        document()->push_command(new command::RemoveObject(
            this,
            &document()->defs()->colors
        ));
        return true;
    }
    return false;
}
