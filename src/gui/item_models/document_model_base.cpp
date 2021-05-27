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

std::pair<model::DocumentNode *, int> item_models::DocumentModelBase::drop_position(const QModelIndex& parent, int row) const
{
    return {node(parent), row};
}

bool item_models::DocumentModelBase::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    Q_UNUSED(column);

    if ( !data || action != Qt::MoveAction || !document() )
        return false;

    if ( !data->hasFormat("application/x.glaxnimate-node-uuid") )
        return false;

    model::DocumentNode* parent_node;
    std::tie(parent_node, row) = drop_position(parent, row);
    if ( !parent_node )
        return false;

    auto dest = static_cast<model::ShapeListProperty*>(parent_node->get_property("shapes"));
    if ( !dest )
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
