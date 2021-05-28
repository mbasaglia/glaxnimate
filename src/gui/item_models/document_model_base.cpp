#include "document_model_base.hpp"

#include <QMimeData>
#include <QDataStream>
#include <QSet>

#include "command/object_list_commands.hpp"
#include "command/undo_macro_guard.hpp"

#include "drag_data.hpp"

Qt::DropActions item_models::DocumentModelBase::supportedDropActions() const
{
    return Qt::MoveAction;
}

QStringList item_models::DocumentModelBase::mimeTypes() const
{
    return {"application/x.glaxnimate-node-uuid"};
}

QMimeData * item_models::DocumentModelBase::mimeData(const QModelIndexList& indexes) const
{
    if ( !document() )
        return nullptr;

    QMimeData *data = new QMimeData();
    item_models::DragEncoder encoder;
    for ( const auto& index : indexes )
        encoder.add_node(node(index));

    data->setData("application/x.glaxnimate-node-uuid", encoder.data());
    return data;
}

std::pair<model::VisualNode *, int> item_models::DocumentModelBase::drop_position(const QModelIndex& parent, int row) const
{
    return {visual_node(parent), row};
}

std::tuple<model::VisualNode *, int, model::ShapeListProperty*> item_models::DocumentModelBase::cleaned_drop_position(const QMimeData* data, Qt::DropAction action, const QModelIndex& parent, int row) const
{
    if ( !data || action != Qt::MoveAction || !document() )
        return {};

    if ( !data->hasFormat("application/x.glaxnimate-node-uuid") )
        return {};

    auto position = drop_position(parent, row);
    model::VisualNode* parent_node = position.first;

    if ( !parent_node )
        return {};

    auto dest = static_cast<model::ShapeListProperty*>(parent_node->get_property("shapes"));
    if ( !dest )
        return {};

    if ( !parent_node || parent_node->docnode_locked_recursive() )
        return {};

    return {position.first, position.second, dest};
}


bool item_models::DocumentModelBase::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    Q_UNUSED(column);

    model::DocumentNode* parent_node;
    model::ShapeListProperty* dest;
    std::tie(parent_node, row, dest) = cleaned_drop_position(data, action, parent, row);
    if ( !parent_node )
        return false;

    int max_child = parent_node->docnode_child_count();
    int insert = max_child - row;

    if ( row > max_child || row < 0)
        insert = max_child;

    DragDecoder<model::ShapeElement> decoder(data->data("application/x.glaxnimate-node-uuid"), document());
    std::vector<model::ShapeElement*> items(decoder.begin(), decoder.end());

    if ( items.empty() )
        return false;

    command::UndoMacroGuard guard(tr("Move Layers"), document());

    for ( auto shape : items )
    {

        document()->push_command(new command::MoveObject(
            shape, shape->owner(), dest, insert
        ));
    }

    return true;
}

bool item_models::DocumentModelBase::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
{
    Q_UNUSED(column);

    return std::get<model::VisualNode*>(cleaned_drop_position(data, action, parent, row));
}
