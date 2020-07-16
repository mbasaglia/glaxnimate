#include "document_node_model.hpp"


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

    if ( index.column() == ColumnName )
        flags |= Qt::ItemIsEditable;
    else
        flags |= Qt::ItemIsUserCheckable;

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
            return {}; /// TODO layer colors
        case ColumnName:
            if ( role == Qt::DisplayRole )
                return n->name.get();
            else if ( role == Qt::DecorationRole )
                return n->docnode_icon();
            break;
        case ColumnVisible:
            if ( role == Qt::DecorationRole )
            {
                if ( n->docnode_visible() )
                    return QIcon::fromTheme("user-online");
                return QIcon::fromTheme("user-invisible");
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
    auto n = node(index);
    if ( !document || !index.isValid() || !n )
        return false;

    switch ( index.column() )
    {
        case ColumnColor:
            return false; /// TODO layer colors
        case ColumnName:
            return n->name.set_value(value);
        case ColumnVisible:
            n->docnode_set_visible(value.toBool());
            return true;
        case ColumnLocked:
            n->docnode_set_locked(value.toBool());
            return true;
    }
    return {};

}



void model::DocumentNodeModel::set_document ( model::Document* doc )
{
    beginResetModel();
    document = doc;
    endResetModel();
}

void model::DocumentNodeModel::clear()
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

    auto parent = n->docnode_parent();
    if ( !parent )
        return {};

    auto granny = parent->docnode_parent();

    if ( !granny )
    {
        // TODO precomps
        return createIndex(1, 0, parent);
    }

    for ( int i = 0; i < granny->docnode_child_count(); i++ )
    {
        if ( granny->docnode_child(i) == parent )
            return createIndex(i, 0, parent);
    }

    return {};
}

bool model::DocumentNodeModel::moveRows ( const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild )
{
    return false;
}

bool model::DocumentNodeModel::removeRows ( int row, int count, const QModelIndex& parent )
{
    return false;
}


