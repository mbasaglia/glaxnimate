#include "precomposition.hpp"
#include "model/document.hpp"
#include "model/assets/assets.hpp"
#include "command/object_list_commands.hpp"

GLAXNIMATE_OBJECT_IMPL(model::Precomposition)

QIcon model::Precomposition::tree_icon() const
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
            &document()->assets()->precompositions->values
        ));
        return true;
    }
    return false;
}

model::DocumentNode * model::Precomposition::docnode_parent() const
{
    return document()->assets()->precompositions.get();
}
