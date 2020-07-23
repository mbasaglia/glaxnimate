#include "document_node_model.hpp"
#include "command/property_commands.hpp"

void model::DocumentNodeModel::connect_node ( model::DocumentNode* node )
{
    connect(node, &model::DocumentNode::docnode_child_add_begin, this, [this, node](int row) {
        beginInsertRows(node_index(node), row, row);
    });
    connect(node, &model::DocumentNode::docnode_child_add_end, this, [this, node](model::DocumentNode* child) {
        endInsertRows();
        connect_node(child);
    });
    connect(node, &model::DocumentNode::docnode_child_remove_begin, this, [this, node](int row) {
        beginRemoveRows(node_index(node), row, row);
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
        dataChanged(changed, changed, {Qt::BackgroundColorRole, Qt::EditRole, Qt::DisplayRole});
    });
    connect(node, &model::DocumentNode::docnode_name_changed, this, [this, node]() {
        QModelIndex ind = node_index(node);
        QModelIndex par = node_index(node->docnode_parent());
        QModelIndex changed = index(ind.row(), ColumnName, par);
        dataChanged(changed, changed, {Qt::EditRole, Qt::DisplayRole});
    });

    for ( DocumentNode* child : node->docnode_children() )
        connect_node(child);
}

void model::DocumentNodeModel::disconnect_node ( model::DocumentNode* node )
{
    disconnect(node, nullptr, this, nullptr);

    for ( DocumentNode* child : node->docnode_children() )
        disconnect_node(child);
}




int model::DocumentNodeModel::rowCount ( const QModelIndex& parent ) const
{
    if ( !document )
        return 0;

    if ( !parent.isValid() )
        return 1; // TODO add precomps

    return node(parent)->docnode_child_count();
}

int model::DocumentNodeModel::columnCount ( const QModelIndex& ) const
{
    return ColumnCount;
}

QModelIndex model::DocumentNodeModel::index ( int row, int column, const QModelIndex& parent ) const
{
    if ( !document )
        return {};

    if ( !parent.isValid() )
    {
        // TODO add precomps
        return createIndex(row, column, &document->animation());
    }

    auto n = node(parent);
    if ( !n || row < 0 || row >= n->docnode_child_count() )
        return {};

    return createIndex(row, column, n->docnode_child(row));
}

Qt::ItemFlags model::DocumentNodeModel::flags ( const QModelIndex& index ) const
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

    return flags;
}

QVariant model::DocumentNodeModel::data(const QModelIndex& index, int role) const
{
    auto n = node(index);
    if ( !document || !index.isValid() || !n )
        return {};

    switch ( index.column() )
    {
        case ColumnColor:
            if ( role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::BackgroundColorRole )
                return n->docnode_group_color();
            break;
        case ColumnName:
            if ( role == Qt::DisplayRole || role == Qt::EditRole )
                return n->docnode_name();
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

bool model::DocumentNodeModel::setData(const QModelIndex& index, const QVariant& value, int role)
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


void model::DocumentNodeModel::set_document ( model::Document* doc )
{
    beginResetModel();
    document = doc;
    if ( doc )
        connect_node(&doc->animation());
    endResetModel();
}

void model::DocumentNodeModel::clear_document()
{
    set_document(nullptr);
}

model::DocumentNode * model::DocumentNodeModel::node ( const QModelIndex& index ) const
{
    return (model::DocumentNode*)index.internalPointer();
}



QModelIndex model::DocumentNodeModel::parent ( const QModelIndex& index ) const
{
    auto n = node(index);
    if ( !document || !index.isValid() || !n )
        return {};

    return node_index(n->docnode_parent());
}

QModelIndex model::DocumentNodeModel::node_index ( model::DocumentNode* node ) const
{
    if ( !node )
        return {};

    auto parent = node->docnode_parent();

    if ( !parent )
    {
        // TODO precomps
        return createIndex(0, 0, node);
    }

    for ( int i = 0; i < parent->docnode_child_count(); i++ )
    {
        if ( parent->docnode_child(i) == node )
            return createIndex(i, 0, node);
    }

    return {};
}

bool model::DocumentNodeModel::moveRows ( const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild )
{
    Q_UNUSED(sourceParent); Q_UNUSED(sourceRow); Q_UNUSED(count); Q_UNUSED(destinationParent); Q_UNUSED(destinationChild);
    return false;
}

bool model::DocumentNodeModel::removeRows ( int row, int count, const QModelIndex& parent )
{
    Q_UNUSED(row); Q_UNUSED(count); Q_UNUSED(parent);
    return false;
}


