#include "precomposition.hpp"
#include "model/document.hpp"
#include "model/defs/defs.hpp"
#include "command/object_list_commands.hpp"

GLAXNIMATE_OBJECT_IMPL(model::Precomposition)

QIcon model::Precomposition::docnode_icon() const
{
    return QIcon::fromTheme("component");
}

QString model::Precomposition::type_name_human() const
{
    return tr("Composition");
}

QRectF model::Precomposition::local_bounding_rect(FrameTime) const
{
    return document()->rect();
}

bool model::Precomposition::remove_if_unused(bool clean_lists)
{
    if ( clean_lists && users().empty() )
    {
        document()->push_command(new command::RemoveObject(
            this,
            &document()->defs()->precompositions
        ));
        return true;
    }
    return false;
}
