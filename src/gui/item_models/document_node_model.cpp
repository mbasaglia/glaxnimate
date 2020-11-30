#include "document_node_model.hpp"

#include <QMimeData>
#include <QDataStream>
#include <QSet>

#include "command/property_commands.hpp"
#include "command/object_list_commands.hpp"
#include "command/undo_macro_guard.hpp"

#include "model/shapes/shape.hpp"
#include "model/defs/defs.hpp"


void item_models::DocumentNodeModel::connect_node ( model::DocumentNode* node )
{
    connect(node, &model::DocumentNode::docnode_child_add_begin, this, [this, node](int row) {
        int rows = node->docnode_child_count();
        beginInsertRows(node_index(node), rows - row, rows - row);
    });
    connect(node, &model::DocumentNode::docnode_child_add_end, this, [this, node](model::DocumentNode* child) {
        endInsertRows();
        connect_node(child);
    });
    connect(node, &model::DocumentNode::docnode_child_remove_begin, this, [this, node](int row) {
        int rows = node->docnode_child_count();
        beginRemoveRows(node_index(node), rows - row - 1, rows - row - 1);
    });
    connect(node, &model::DocumentNode::docnode_child_remove_end, this, [this, node](model::DocumentNode* child) {
        endRemoveRows();
        disconnect_node(child);
    });
    connect(node, &model::DocumentNode::docnode_visible_changed, this, [this, node]() {
        QModelIndex ind = node_index(node);
        QModelIndex par = node_index(node->docnode_parent());
        QModelIndex changed = index(ind.row(), ColumnVisible, par);
        dataChanged(changed, changed, {Qt::DecorationRole});
    });
    connect(node, &model::DocumentNode::docnode_locked_changed, this, [this, node]() {
        QModelIndex ind = node_index(node);
        QModelIndex par = node_index(node->docnode_parent());
        QModelIndex changed = index(ind.row(), ColumnLocked, par);
        dataChanged(changed, changed, {Qt::DecorationRole});
    });
    connect(node, &model::DocumentNode::docnode_group_color_changed, this, [this, node]() {
        QModelIndex ind = node_index(node);
        QModelIndex par = node_index(node->docnode_parent());
        QModelIndex changed = index(ind.row(), ColumnColor, par);
        dataChanged(changed, changed, {Qt::BackgroundRole, Qt::EditRole, Qt::DisplayRole});
    });
    connect(node, &model::DocumentNode::name_changed, this, [this, node]() {
        QModelIndex ind = node_index(node);
        QModelIndex par = node_index(node->docnode_parent());
        QModelIndex changed = index(ind.row(), ColumnName, par);
        dataChanged(changed, changed, {Qt::EditRole, Qt::DisplayRole});
    });
    connect(node, &model::DocumentNode::docnode_child_move_begin, this, [this, node](int a, int b) {
        int rows = node->docnode_child_count();

        int src = rows - a - 1;
        int dest = rows - b - 1;
        if ( src < dest )
            dest++;

        QModelIndex parent = node_index(node);
        beginMoveRows(parent, src, src, parent, dest);
    });
    connect(node, &model::DocumentNode::docnode_child_move_end, this, [this]() {
        endMoveRows();
    });

    for ( model::DocumentNode* child : node->docnode_children() )
        connect_node(child);
}

void item_models::DocumentNodeModel::disconnect_node ( model::DocumentNode* node )
{
    disconnect(node, nullptr, this, nullptr);

    for ( model::DocumentNode* child : node->docnode_children() )
        disconnect_node(child);
}

int item_models::DocumentNodeModel::rowCount ( const QModelIndex& parent ) const
{
    if ( !document )
        return 0;

    if ( !parent.isValid() )
        return 1 + document->defs()->precompositions.size();

    return node(parent)->docnode_child_count();
}

int item_models::DocumentNodeModel::columnCount ( const QModelIndex& ) const
{
    return ColumnCount;
}

QModelIndex item_models::DocumentNodeModel::index ( int row, int column, const QModelIndex& parent ) const
{
    if ( !document )
        return {};

    if ( !parent.isValid() )
    {
        int i = row - 1;
        if ( i >= 0 && i < document->defs()->precompositions.size() )
            return createIndex(row, column, document->defs()->precompositions[i]);
        return createIndex(0, column, document->main());
    }

    auto n = node(parent);
    int rows = n->docnode_child_count();
    if ( !n || row < 0 || row >= rows )
        return {};

    return createIndex(row, column, n->docnode_child(rows - row - 1));
}

Qt::ItemFlags item_models::DocumentNodeModel::flags ( const QModelIndex& index ) const
{
    if ( !document )
        return Qt::NoItemFlags;


    Qt::ItemFlags flags = QAbstractItemModel::flags(index);

    switch ( index.column() )
    {
        case ColumnName:
        case ColumnColor:
            flags |= Qt::ItemIsEditable;
            break;
//         case ColumnLocked:
//         case ColumnVisible:
//             flags |= Qt::ItemIsUserCheckable;
//             break;
    }

    auto n = node(index);
    if ( n && !n->docnode_locked_recursive() )
    {
        if ( n->has("shapes") )
            flags |= Qt::ItemIsDropEnabled;
        if ( !qobject_cast<model::MainComposition*>(n) )
            flags |= Qt::ItemIsDragEnabled;
    }

    return flags;
}

QVariant item_models::DocumentNodeModel::data(const QModelIndex& index, int role) const
{
    auto n = node(index);
    if ( !document || !index.isValid() || !n )
        return {};

    switch ( index.column() )
    {
        case ColumnColor:
            if ( role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::BackgroundRole )
                return n->docnode_group_color();
            break;
        case ColumnName:
            if ( role == Qt::DisplayRole || role == Qt::EditRole )
                return n->object_name();
            else if ( role == Qt::DecorationRole )
                return n->docnode_icon();
            break;
        case ColumnVisible:
            if ( role == Qt::DecorationRole )
            {
                if ( n->visible.get() )
                    return QIcon::fromTheme("view-visible");
                return QIcon::fromTheme("view-hidden");
            }
            break;
        case ColumnLocked:
            if ( role == Qt::DecorationRole )
            {
                if ( n->locked.get() )
                    return QIcon::fromTheme("object-locked");
                return QIcon::fromTheme("object-unlocked");
            }
            break;
    }
    if ( role == Qt::UserRole )
        return n->uuid.get();
    return {};
}

bool item_models::DocumentNodeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    Q_UNUSED(role);
    auto n = node(index);
    if ( !document || !index.isValid() || !n )
        return false;

    switch ( index.column() )
    {
        case ColumnColor:
            document->undo_stack().push(new command::SetPropertyValue(&n->group_color, n->group_color.get(), value));
            return true;
        case ColumnName:
            document->undo_stack().push(new command::SetPropertyValue(&n->name, n->name.get(), value));
            return true;
    }

    return false;
}


void item_models::DocumentNodeModel::set_document ( model::Document* doc )
{
    beginResetModel();

    if ( document )
    {
        disconnect(document->defs(), nullptr, this, nullptr);
    }

    document = doc;

    if ( doc )
    {
        connect_node(doc->main());
        for ( const auto& comp : doc->defs()->precompositions )
            connect_node(comp.get());

        connect(doc->defs(), &model::Defs::precomp_add_begin, this, [this](int row){
            beginInsertRows({}, row+1, row+1);
        });
        connect(doc->defs(), &model::Defs::precomp_add_end, this, [this](model::Precomposition* precomp){
            endInsertRows();
            connect_node(precomp);
        });
        connect(doc->defs(), &model::Defs::precomp_remove_begin, this, [this](int row){
            beginRemoveRows({}, row+1, row+1);
        });
        connect(doc->defs(), &model::Defs::precomp_remove_end, this, [this](model::Precomposition* precomp){
            endRemoveRows();
            disconnect_node(precomp);
        });
        connect(doc->defs(), &model::Defs::precomp_move_begin, this, [this](int from, int to){
            beginMoveRows({}, from+1, from+1, {}, to+1);
        });
        connect(doc->defs(), &model::Defs::precomp_move_end, this, [this](){
            endMoveRows();
        });
    }
    endResetModel();
}

void item_models::DocumentNodeModel::clear_document()
{
    set_document(nullptr);
}

model::DocumentNode * item_models::DocumentNodeModel::node ( const QModelIndex& index ) const
{
    return (model::DocumentNode*)index.internalPointer();
}

QModelIndex item_models::DocumentNodeModel::parent ( const QModelIndex& index ) const
{
    auto n = node(index);
    if ( !document || !index.isValid() || !n )
        return {};

    return node_index(n->docnode_parent());
}

QModelIndex item_models::DocumentNodeModel::node_index ( model::DocumentNode* node ) const
{
    if ( !node )
        return {};

    auto parent = node->docnode_parent();

    if ( !parent )
    {
        if ( !document )
            return {};

        if ( node == document->main() )
            return createIndex(0, 0, node);

        return createIndex(
            document->defs()->precompositions.index_of(static_cast<model::Precomposition*>(node))+1,
            0, node
        );
    }

    int rows = parent->docnode_child_count();
    for ( int i = 0; i < rows; i++ )
    {
        if ( parent->docnode_child(i) == node )
            return createIndex(rows - i - 1, 0, node);
    }

    return {};
}

bool item_models::DocumentNodeModel::moveRows ( const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild )
{
    Q_UNUSED(sourceParent); Q_UNUSED(sourceRow); Q_UNUSED(count); Q_UNUSED(destinationParent); Q_UNUSED(destinationChild);
    return false;
}

bool item_models::DocumentNodeModel::removeRows ( int row, int count, const QModelIndex& parent )
{
    Q_UNUSED(row); Q_UNUSED(count); Q_UNUSED(parent);
    return false;
}

Qt::DropActions item_models::DocumentNodeModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QStringList item_models::DocumentNodeModel::mimeTypes() const
{
    return {"application/x.glaxnimate-node-uuid"};
}

QMimeData * item_models::DocumentNodeModel::mimeData(const QModelIndexList& indexes) const
{
    if ( !document )
        return nullptr;

    QMimeData *data = new QMimeData();
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);
    QSet<QUuid> uniq;
    for ( const auto& index : indexes )
    {
        if ( auto n = node(index) )
        {
            if ( !uniq.contains(n->uuid.get()) )
            {
                stream << n->uuid.get();
                uniq.insert(n->uuid.get());
            }
        }
    }
    data->setData("application/x.glaxnimate-node-uuid", encoded);
    return data;
}

bool item_models::DocumentNodeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    Q_UNUSED(column);

    if ( !data || action != Qt::MoveAction || !document )
        return false;

    if ( !data->hasFormat("application/x.glaxnimate-node-uuid") )
        return false;

    auto parent_node = node(parent);
    if ( !parent_node )
        return false;

    auto dest = static_cast<model::ShapeListProperty*>(parent_node->get_property("shapes"));
    if ( !dest )
        return false;

    int max_child = parent_node->docnode_child_count();
    int insert = max_child - row;

    if ( row > max_child || row < 0)
        insert = max_child;


    QByteArray encoded = data->data("application/x.glaxnimate-node-uuid");
    QDataStream stream(&encoded, QIODevice::ReadOnly);

    command::UndoMacroGuard guard(tr("Move Layers"), document);

    while ( !stream.atEnd() )
    {
        QUuid uuid;
        stream >> uuid;
        auto n = document->find_by_uuid(uuid);
        if ( !n )
            continue;
        auto shape = n->cast<model::ShapeElement>();
        if ( !shape )
            continue;

        document->push_command(new command::MoveObject(
            shape, shape->owner(), dest, insert
        ));
    }
    return true;
}
