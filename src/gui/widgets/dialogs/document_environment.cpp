#include "document_environment.hpp"

#include "command/shape_commands.hpp"
#include "command/structure_commands.hpp"
#include "command/undo_macro_guard.hpp"

model::ShapeElement* glaxnimate::gui::DocumentEnvironment::current_shape()
{
    model::DocumentNode* curr = current_document_node();
    if ( curr )
    {
        if ( auto curr_shape = qobject_cast<model::ShapeElement*>(curr) )
            return curr_shape;
    }
    return nullptr;
}

model::ShapeListProperty* glaxnimate::gui::DocumentEnvironment::current_shape_container()
{
    model::DocumentNode* sh = current_document_node();
    if ( auto lay = qobject_cast<model::Composition*>(sh) )
        return &lay->shapes;

    if ( !qobject_cast<model::Layer*>(sh) )
        sh = sh->docnode_parent();

    while ( sh )
    {
        if ( auto grp = qobject_cast<model::Group*>(sh) )
            return &grp->shapes;
        if ( auto lay = qobject_cast<model::Composition*>(sh) )
            return &lay->shapes;
        sh = sh->docnode_parent();
    }
    return &current_composition()->shapes;
}

void glaxnimate::gui::DocumentEnvironment::delete_selected()
{
    auto selection = cleaned_selection();
    if ( selection.empty() )
        return;

    auto doc = document();

    command::UndoMacroGuard macro(QObject::tr("Delete"), doc);
    for ( auto item : selection )
    {
        if ( auto shape = qobject_cast<model::ShapeElement*>(item) )
            if ( !shape->locked.get() )
                doc->push_command(new command::RemoveShape(shape, shape->owner()));
    }
}
