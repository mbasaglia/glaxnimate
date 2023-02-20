/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "structure_commands.hpp"
#include "shape_commands.hpp"

using namespace glaxnimate;

command::DeferredCommandBase::~DeferredCommandBase() = default;

bool command::DeferredCommandBase::has_action() const
{
    return bool(d);
}

void command::DeferredCommandBase::redo()
{
    if ( d )
        d->redo();
}

void command::DeferredCommandBase::undo()
{
    if ( d )
        d->undo();
}


bool command::ReorderCommand::resolve_position(model::ShapeElement* shape, int& new_position)
{
    if ( new_position < 0 )
    {
        switch ( command::ReorderCommand::SpecialPosition(new_position) )
        {
            case command::ReorderCommand::MoveUp:
                new_position = shape->position() + 1;
                break;
            case command::ReorderCommand::MoveDown:
                new_position = shape->position() - 1;
                break;
            case command::ReorderCommand::MoveTop:
                new_position = shape->owner()->size() - 1;
                break;
            case command::ReorderCommand::MoveBottom:
                new_position = 0;
                break;
        }
    }

    if ( new_position == shape->position() || new_position < 0 || new_position >= shape->owner()->size() )
        return false;
    return true;
}

std::unique_ptr<QUndoCommand> reorder_shape(model::ShapeElement* shape, int new_position)
{
    if ( !command::ReorderCommand::resolve_position(shape, new_position) )
        return {};
    return std::make_unique<command::MoveShape>(shape, shape->owner(), shape->owner(), new_position);
}

command::ReorderCommand::ReorderCommand(model::ShapeElement* shape, int new_position)
    : DeferredCommandBase(name(shape))
{
    d = reorder_shape(shape, new_position);
}

QString command::ReorderCommand::name(model::DocumentNode* node)
{
    return QObject::tr("Move %1").arg(node->object_name());
}

