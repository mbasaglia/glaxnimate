#include "document_node_model.hpp"
#include "command/property_commands.hpp"

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
    connect(node, &model::DocumentNode::docnode_name_changed, this, [this, node]() {
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
        return 1; // TODO add precomps

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
        // TODO add precomps
        return createIndex(row, column, document->main_composition());
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
                if ( n->docnode_visible() )
                    return QIcon::fromTheme("view-visible");
                return QIcon::fromTheme("view-hidden");
            }
            break;
        case ColumnLocked:
            if ( role == Qt::DecorationRole )
            {
                if ( n->docnode_locked() )
                    return QIcon::fromTheme("object-locked");
                return QIcon::fromTheme("object-unlocked");
            }
            break;
    }
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
    document = doc;
    if ( doc )
        connect_node(doc->main_composition());
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
        // TODO precomps
        return createIndex(0, 0, node);
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

