#include "structure_commands.hpp"
#include "layer_commands.hpp"
#include "shape_commands.hpp"


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

static std::unique_ptr<QUndoCommand> duplicate_layer(model::Layer* lay)
{
    auto new_layer = lay->clone_covariant();
    new_layer->recursive_rename();
    return std::make_unique<command::AddLayer>(
        lay->composition(),
        std::move(new_layer),
        lay->composition()->docnode_child_index(lay)
    );
}

static std::unique_ptr<QUndoCommand> duplicate_shape(model::ShapeElement* shape)
{
    std::unique_ptr<model::ShapeElement> new_shape (
        static_cast<model::ShapeElement*>(shape->clone().release())
    );
    new_shape->recursive_rename();
    return std::make_unique<command::AddShape>(
        shape->owner(),
        std::move(new_shape),
        shape->owner()->index_of(shape)
    );
}

QString command::DuplicateCommand::name(model::DocumentNode* node)
{
    return QObject::tr("Duplicate %1").arg(node->object_name());
}

command::DuplicateCommand::DuplicateCommand(model::DocumentNode* node)
    : DeferredCommandBase(name(node))
{
    if ( auto lay = qobject_cast<model::Layer*>(node) )
        d = duplicate_layer(lay);
    else if ( auto shape = qobject_cast<model::ShapeElement*>(node) )
        d = duplicate_shape(shape);
}

command::DuplicateCommand::DuplicateCommand(model::Layer* node)
    : DeferredCommandBase(name(node), duplicate_layer(node))
{}

command::DuplicateCommand::DuplicateCommand(model::ShapeElement* node)
    : DeferredCommandBase(name(node), duplicate_shape(node))
{}

static std::unique_ptr<QUndoCommand> delete_layer(model::Layer* lay)
{
    return std::make_unique<command::RemoveLayer>(lay);
}

static std::unique_ptr<QUndoCommand> delete_shape(model::ShapeElement* shape)
{
    return std::make_unique<command::RemoveShape>(shape);
}

QString command::DeleteCommand::name(model::DocumentNode* node)
{
    return QObject::tr("Delete %1").arg(node->object_name());
}

command::DeleteCommand::DeleteCommand(model::DocumentNode* node)
    : DeferredCommandBase(name(node))
{
    if ( auto lay = qobject_cast<model::Layer*>(node) )
        d = delete_layer(lay);
    else if ( auto shape = qobject_cast<model::ShapeElement*>(node) )
        d = delete_shape(shape);
}

command::DeleteCommand::DeleteCommand(model::Layer* node)
    : DeferredCommandBase(name(node), delete_layer(node))
{}

command::DeleteCommand::DeleteCommand(model::ShapeElement* node)
    : DeferredCommandBase(name(node), delete_shape(node))
{}


bool command::ReorderCommand::resolve_position(model::Layer* lay, int& new_position)
{
    int old_position = lay->composition()->layers.index_of(lay);

    if ( new_position < 0 )
    {
        switch ( command::ReorderCommand::SpecialPosition(new_position) )
        {
            case command::ReorderCommand::MoveUp:
                new_position = old_position + 1;
                break;
            case command::ReorderCommand::MoveDown:
                new_position = old_position - 1;
                break;
            case command::ReorderCommand::MoveTop:
                new_position = lay->composition()->layers.size() - 1;
                break;
            case command::ReorderCommand::MoveBottom:
                new_position = 0;
                break;
        }
    }

    if ( new_position == old_position || new_position < 0 || new_position >= lay->composition()->layers.size() )
        return false;

    return true;
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

bool command::ReorderCommand::resolve_position(model::DocumentNode* node, int& position)
{
    if ( auto lay = qobject_cast<model::Layer*>(node) )
        return resolve_position(lay, position);
    else if ( auto shape = qobject_cast<model::ShapeElement*>(node) )
        return resolve_position(shape, position);
    return false;
}


static std::unique_ptr<QUndoCommand> reorder_layer(model::Layer* lay, int new_position)
{
    if ( !command::ReorderCommand::resolve_position(lay, new_position) )
        return {};
    return std::make_unique<command::MoveLayer>(lay, lay->composition(), new_position);
}

std::unique_ptr<QUndoCommand> reorder_shape(model::ShapeElement* shape, int new_position)
{
    if ( !command::ReorderCommand::resolve_position(shape, new_position) )
        return {};
    return std::make_unique<command::MoveShape>(shape, shape->owner(), new_position);
}

command::ReorderCommand::ReorderCommand(model::DocumentNode* node, int new_position)
    : DeferredCommandBase(name(node))
{
    if ( auto lay = qobject_cast<model::Layer*>(node) )
        d = reorder_layer(lay, new_position);
    else if ( auto shape = qobject_cast<model::ShapeElement*>(node) )
        d = reorder_shape(shape, new_position);
}

command::ReorderCommand::ReorderCommand(model::Layer* lay, int new_position)
    : DeferredCommandBase(name(lay))
{
    d = reorder_layer(lay, new_position);
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

