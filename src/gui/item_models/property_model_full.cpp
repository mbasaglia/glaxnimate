#include "property_model_full.hpp"
#include "property_model_private.hpp"

#include "model/stretchable_time.hpp"

class item_models::PropertyModelFull::Private : public PropertyModelBase::Private
{
public:
    using PropertyModelBase::Private::Private;

    PropertyModelFull* my_model()
    {
        return static_cast<PropertyModelFull*>(model);
    }

    void on_connect(model::Object* object, Subtree* tree) override
    {
        if ( auto visual = object->cast<model::VisualNode>() )
        {
            connect(visual, &model::VisualNode::docnode_visible_changed, model, [this, visual]() {
                QModelIndex ind = node_index(visual);
                QModelIndex par = node_index(visual->docnode_parent());
                QModelIndex changed = model->index(ind.row(), ColumnVisible, par);
                model->dataChanged(changed, changed, {Qt::DecorationRole});
            });
            connect(visual, &model::VisualNode::docnode_locked_changed, model, [this, visual]() {
                QModelIndex ind = node_index(visual);
                QModelIndex par = node_index(visual->docnode_parent());
                QModelIndex changed = model->index(ind.row(), ColumnLocked, par);
                model->dataChanged(changed, changed, {Qt::DecorationRole});
            });
            connect(visual, &model::VisualNode::docnode_group_color_changed, model, [this, visual]() {
                QModelIndex ind = node_index(visual);
                QModelIndex par = node_index(visual->docnode_parent());
                QModelIndex changed = model->index(ind.row(), ColumnColor, par);
                model->dataChanged(changed, changed, {Qt::BackgroundRole, Qt::EditRole, Qt::DisplayRole});
            });
            connect(visual, &model::VisualNode::name_changed, model, [this, visual]() {
                QModelIndex ind = node_index(visual);
                QModelIndex par = node_index(visual->docnode_parent());
                QModelIndex changed = model->index(ind.row(), ColumnName, par);
                model->dataChanged(changed, changed, {Qt::EditRole, Qt::DisplayRole});
            });
        }

        std::vector<model::BaseProperty*> object_lists;

        for ( model::BaseProperty* prop : object->properties() )
        {
            if (
                (prop->traits().flags & model::PropertyTraits::List) &&
                prop->traits().type == model::PropertyTraits::Object
            )
            {
                object_lists.push_back(static_cast<model::ObjectListPropertyBase*>(prop));
            }
            else if ( prop->traits().type == model::PropertyTraits::Object )
            {
                model::Object* subobj = prop->value().value<model::Object*>();
                if ( subobj )
                {
                    auto meta = subobj->metaObject();
                    if (
                        !meta->inherits(&model::AnimationContainer::staticMetaObject) &&
                        !meta->inherits(&model::StretchableTime::staticMetaObject) &&
                        !meta->inherits(&model::MaskSettings::staticMetaObject)
                    )
                        connect_subobject(subobj, tree);
                }
            }
            else if ( prop->traits().flags & model::PropertyTraits::Visual &&
                prop->traits().type != model::PropertyTraits::ObjectReference && prop->name() != "visible" )
            {
                properties[prop] = add_node(Subtree{prop, tree->id})->id;
            }
        }


        Subtree* child = nullptr;

        // Show object lists at the end
        for ( auto prop : object_lists )
        {
            Subtree* prop_node = add_node(Subtree{prop, tree->id});
            child = prop_node;
            prop_node->prop_value = prop->value().toList();
            properties[prop] = prop_node->id;
            connect_list(prop_node);
        }

        if ( child )
        {
            if ( auto node = tree->object->cast<model::DocumentNode>() )
                connect_docnode(node, child);
        }
    }

    void connect_list(Subtree* prop_node)
    {
        if ( prop_node->prop->traits().is_object() )
        {
            for ( int i = 0; i < prop_node->prop_value.size(); i++ )
            {
                model::Object* subobj = prop_node->prop_value[i].value<model::Object*>();
                auto suboj_node = add_node(Subtree{subobj, prop_node->id});
                connect_recursive(suboj_node);
            }
        }
        else
        {
            for ( int i = 0; i < prop_node->prop_value.size(); i++ )
            {
                add_node(Subtree{i, prop_node->id});
            }
        }
    }

    void connect_docnode(model::DocumentNode* node, Subtree* insert_into)
    {
        auto id = insert_into->id;
        connect(node, &model::DocumentNode::docnode_child_add_begin, model, [this, node, id](int row) {
            int rows = node->docnode_child_count();
            my_model()->beginInsertRows(subtree_index(id), rows - row, rows - row);
        });
        connect(node, &model::DocumentNode::docnode_child_add_end, model, [this, insert_into, id](model::DocumentNode* child) {
            add_object(child, insert_into, false);
            my_model()->endInsertRows();
        });
        connect(node, &model::DocumentNode::docnode_child_remove_begin, model, [this, node](int row) {
            int rows = node->docnode_child_count();
            my_model()->beginRemoveRows(node_index(node), rows - row - 1, rows - row - 1);
        });
        connect(node, &model::DocumentNode::docnode_child_remove_end, model, [this, node](model::DocumentNode* child) {
            if ( auto tree = object_tree(child) )
                disconnect_recursive(tree);
            my_model()->endRemoveRows();
        });
    }

    QVariant data_color(Subtree* tree, int role)
    {
        if ( tree->visual_node && ( role == Qt::DisplayRole || role == Qt::EditRole ) )
            return tree->visual_node->docnode_group_color();

        return {};
    }

    QVariant data_visible(Subtree* tree, int role)
    {
        if ( tree->visual_node && ( role == Qt::DecorationRole ) )
        {
            if ( tree->visual_node->visible.get() )
                return QIcon::fromTheme("view-visible");
            return QIcon::fromTheme("view-hidden");
        }

        return {};
    }

    QVariant data_locked(Subtree* tree, int role)
    {
        if ( tree->visual_node && ( role == Qt::DecorationRole ) )
        {
            if ( tree->visual_node->locked.get() )
                return QIcon::fromTheme("object-locked");
            return QIcon::fromTheme("object-unlocked");
        }

        return {};
    }
};


item_models::PropertyModelFull::PropertyModelFull()
    : PropertyModelBase(std::make_unique<Private>(this))
{}

item_models::PropertyModelFull::Private* item_models::PropertyModelFull::dd() const
{
    return static_cast<Private*>(d.get());
}

Qt::ItemFlags item_models::PropertyModelFull::flags(const QModelIndex& index) const
{
    if ( d->roots.empty() || !index.isValid() )
        return {};

    Private::Subtree* tree = d->node_from_index(index);
    if ( !tree )
        return {};

    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    switch ( index.column() )
    {
        case ColumnColor:
        case ColumnName:
            if ( tree->visual_node )
                flags |= Qt::ItemIsEditable;
            return flags;
        case ColumnLocked:
        case ColumnVisible:
            return flags;
        case ColumnValue:
            if ( tree->prop )
            {
                model::PropertyTraits traits = tree->prop->traits();

                if ( (traits.flags & (model::PropertyTraits::List|model::PropertyTraits::ReadOnly))
                    || traits.type == model::PropertyTraits::Object || traits.type == model::PropertyTraits::Unknown )
                    return flags;

                if ( traits.type == model::PropertyTraits::Bool )
                    return flags | Qt::ItemIsUserCheckable;

                return flags | Qt::ItemIsEditable;
            }
            return flags;
    }

    return {};
}

QVariant item_models::PropertyModelFull::data(const QModelIndex& index, int role) const
{
    if ( d->roots.empty() || !index.isValid() )
        return {};


    Private::Subtree* tree = d->node_from_index(index);
    if ( !tree )
        return {};

    switch ( index.column() )
    {
        case ColumnName: return d->data_name(tree, role);
        case ColumnValue: return d->data_value(tree, role);
        case ColumnColor: return dd()->data_color(tree, role);
        case ColumnLocked: return dd()->data_locked(tree, role);
        case ColumnVisible: return dd()->data_visible(tree, role);
    }
    return {};
}

bool item_models::PropertyModelFull::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if ( d->roots.empty() || !index.isValid() )
        return false;

    Private::Subtree* tree = d->node_from_index(index);
    if ( !tree )
        return false;

    if ( index.column() == ColumnValue )
    {
        return d->set_prop_data(tree, value, role);
    }
    else if ( tree->visual_node && role == Qt::EditRole )
    {
        switch ( index.column() )
        {
            case ColumnName:
                return tree->visual_node->name.set_undoable(value);
            case ColumnColor:
                return tree->visual_node->group_color.set_undoable(value);
        }
    }

    return false;
}

QVariant item_models::PropertyModelFull::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( orientation == Qt::Horizontal )
    {
        switch ( section )
        {
            case ColumnName:
                if ( role == Qt::DisplayRole )
                    return tr("Name");
                break;
            case ColumnValue:
                if ( role == Qt::DisplayRole )
                    return tr("Value");
                break;
            case ColumnColor:
                if ( role == Qt::ToolTipRole )
                    return tr("Group Color");
                break;
            case ColumnLocked:
                if ( role == Qt::ToolTipRole )
                    return tr("Locked");
                break;
            case ColumnVisible:
                if ( role == Qt::ToolTipRole )
                    return tr("Visible");
                break;
        }
    }
    return {};
}

void item_models::PropertyModelFull::on_document_reset()
{
    if ( d->document )
        d->add_object(d->document->main(), nullptr, false);
}

int item_models::PropertyModelFull::columnCount(const QModelIndex &) const
{
    return ColumnCount;
}
