#include "document_node_model.hpp"

#include <QMimeData>
#include <QDataStream>
#include <QSet>

#include "command/property_commands.hpp"

#include "model/shapes/shape.hpp"
#include "model/assets/assets.hpp"

using namespace glaxnimate::gui;
using namespace glaxnimate;


class item_models::DocumentNodeModel::Private
{
public:
    model::Document* document = nullptr;


    // Sometimes views send QModelIndex instances after they've been removed...
    // So we avoid accessing the internal pointer directly
    std::unordered_set<void*> ptrs;
};

item_models::DocumentNodeModel::DocumentNodeModel(QObject* parent)
    : DocumentModelBase(parent), d(std::make_unique<Private>())
{
}

item_models::DocumentNodeModel::~DocumentNodeModel()
{
}

void item_models::DocumentNodeModel::connect_node ( model::DocumentNode* node )
{
    d->ptrs.insert(node);

    connect(node, &model::DocumentNode::docnode_child_add_begin, this, [this, node](int row) {
        int rows = node->docnode_child_count();
        beginInsertRows(node_index(node), rows - row, rows - row);
    });
    connect(node, &model::DocumentNode::docnode_child_add_end, this, [this](model::DocumentNode* child) {
        endInsertRows();
        connect_node(child);
    });
    connect(node, &model::DocumentNode::docnode_child_remove_begin, this, [this, node](int row) {
        int rows = node->docnode_child_count();
        beginRemoveRows(node_index(node), rows - row - 1, rows - row - 1);
    });
    connect(node, &model::DocumentNode::docnode_child_remove_end, this, [this](model::DocumentNode* child) {
        endRemoveRows();
        disconnect_node(child);
    });
    if ( auto visual = node->cast<model::VisualNode>() )
    {
        connect(visual, &model::VisualNode::docnode_visible_changed, this, [this, visual]() {
            QModelIndex ind = node_index(visual);
            QModelIndex par = node_index(visual->docnode_parent());
            QModelIndex changed = index(ind.row(), ColumnVisible, par);
            dataChanged(changed, changed, {Qt::DecorationRole});
        });
        connect(visual, &model::VisualNode::docnode_locked_changed, this, [this, visual]() {
            QModelIndex ind = node_index(visual);
            QModelIndex par = node_index(visual->docnode_parent());
            QModelIndex changed = index(ind.row(), ColumnLocked, par);
            dataChanged(changed, changed, {Qt::DecorationRole});
        });
        connect(visual, &model::VisualNode::docnode_group_color_changed, this, [this, visual]() {
            QModelIndex ind = node_index(visual);
            QModelIndex par = node_index(visual->docnode_parent());
            QModelIndex changed = index(ind.row(), ColumnColor, par);
            dataChanged(changed, changed, {Qt::BackgroundRole, Qt::EditRole, Qt::DisplayRole});
        });
    }
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

    connect(node, &QObject::destroyed, this, [this, node]{
        d->ptrs.erase(node);
    });
}

void item_models::DocumentNodeModel::disconnect_node ( model::DocumentNode* node )
{
    d->ptrs.erase(node);

    disconnect(node, nullptr, this, nullptr);

    for ( model::DocumentNode* child : node->docnode_children() )
        disconnect_node(child);
}

int item_models::DocumentNodeModel::rowCount ( const QModelIndex& parent ) const
{
    if ( !d->document )
        return 0;

    if ( !parent.isValid() )
        return 2;

    return node(parent)->docnode_child_count();
}

int item_models::DocumentNodeModel::columnCount ( const QModelIndex& ) const
{
    return ColumnCount;
}

QModelIndex item_models::DocumentNodeModel::index ( int row, int column, const QModelIndex& parent ) const
{
    if ( !d->document )
        return {};

    if ( !parent.isValid() )
    {
        if ( row == 0 )
            return createIndex(row, column, d->document->main());
        if ( row == 1 )
            return createIndex(row, column, d->document->assets());
        return {};
    }

    auto n = node(parent);
    int rows = n->docnode_child_count();
    if ( !n || row < 0 || row >= rows )
        return {};

    return createIndex(row, column, n->docnode_child(rows - row - 1));
}

Qt::ItemFlags item_models::DocumentNodeModel::flags ( const QModelIndex& index ) const
{
    if ( !d->document )
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

    auto n = qobject_cast<model::VisualNode*>(node(index));
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
    if ( !d->document || !index.isValid() || !n )
        return {};

    model::VisualNode* visual = nullptr;

// Can't figure how to force this from the style
#ifdef Q_OS_ANDROID
    if ( role == Qt::SizeHintRole && index.column() == ColumnVisible )
        return QSize(80, 80);
#endif

    switch ( index.column() )
    {
        case ColumnColor:
            visual = n->cast<model::VisualNode>();
            if ( visual && (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::BackgroundRole) )
                return visual->docnode_group_color();
            break;
        case ColumnName:
            if ( role == Qt::DisplayRole || role == Qt::EditRole )
                return n->object_name();
#ifndef Q_OS_ANDROID
            else if ( role == Qt::DecorationRole )
                return n->tree_icon();
#endif
            break;
        case ColumnVisible:
            visual = n->cast<model::VisualNode>();
            if ( visual && role == Qt::DecorationRole )
            {
                if ( visual->visible.get() )
                    return QIcon::fromTheme("view-visible");
                return QIcon::fromTheme("view-hidden");
            }
            break;
        case ColumnLocked:
            visual = n->cast<model::VisualNode>();
            if ( visual && role == Qt::DecorationRole )
            {
                if ( visual->locked.get() )
                    return QIcon::fromTheme("object-locked");
                return QIcon::fromTheme("object-unlocked");
            }
            break;
        case ColumnUsers:
            if ( role == Qt::DisplayRole )
                return int(n->users().size());
            break;
    }
    if ( role == Qt::UserRole )
        return n->uuid.get();
    return {};
}

QVariant item_models::DocumentNodeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( orientation == Qt::Horizontal && role == Qt::DisplayRole )
    {
        switch ( section )
        {
            case ColumnColor:
                return tr("Color");
            case ColumnName:
                return tr("Name");
            case ColumnVisible:
                return tr("Visible");
            case ColumnLocked:
                return tr("Locked");
            case ColumnUsers:
                return tr("#");
        }
    }

    return {};
}

bool item_models::DocumentNodeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    Q_UNUSED(role);
    auto n = node(index);
    if ( !d->document || !index.isValid() || !n )
        return false;

    switch ( index.column() )
    {
        case ColumnColor:
            if ( auto visual = n->cast<model::VisualNode>() )
            {
                d->document->undo_stack().push(new command::SetPropertyValue(&visual->group_color, visual->group_color.get(), value));
                return true;
            }
            return false;
        case ColumnName:
            d->document->undo_stack().push(new command::SetPropertyValue(&n->name, n->name.get(), value));
            return true;
    }

    return false;
}

void item_models::DocumentNodeModel::set_document ( model::Document* doc )
{
    beginResetModel();

    if ( d->document )
    {
        disconnect(d->document->assets(), nullptr, this, nullptr);
    }

    d->document = doc;

    if ( doc )
    {
        connect_node(doc->main());
        connect_node(doc->assets());
    }
    endResetModel();
}

void item_models::DocumentNodeModel::clear_document()
{
    set_document(nullptr);
}

model::DocumentNode * item_models::DocumentNodeModel::node ( const QModelIndex& index ) const
{
    auto ptr = (model::DocumentNode*)index.internalPointer();
    if ( d->ptrs.count(ptr) )
        return ptr;
    return nullptr;
}

QModelIndex item_models::DocumentNodeModel::parent ( const QModelIndex& index ) const
{
    auto n = node(index);
    if ( !d->document || !index.isValid() || !n )
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
        if ( !d->document )
            return {};

        if ( node == d->document->main() )
            return createIndex(0, 0, node);

        return createIndex(
            d->document->assets()->precompositions->values.index_of(static_cast<model::Precomposition*>(node))+2,
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

model::VisualNode* item_models::DocumentNodeModel::visual_node(const QModelIndex& index) const
{
    return qobject_cast<model::VisualNode*>(node(index));
}

model::Document * item_models::DocumentNodeModel::document() const
{
    return d->document;
}
