#include "property_model_full.hpp"
#include "property_model_private.hpp"

#include "model/stretchable_time.hpp"
#include "model/assets/assets.hpp"

class item_models::PropertyModelFull::Private : public PropertyModelBase::Private
{
public:
    using PropertyModelBase::Private::Private;

    PropertyModelFull* my_model()
    {
        return static_cast<PropertyModelFull*>(model);
    }

    void on_connect_object_list(Subtree* prop_node, model::DocumentNode* node, model::ObjectListPropertyBase* prop)
    {
        prop_node->merged_children_offset = prop_node->children.size();

        QVariantList prop_value = prop->value().toList();
        for ( auto it = prop_value.rbegin(); it != prop_value.rend(); ++it )
        {
            model::Object* subobj = it->value<model::Object*>();
            auto suboj_node = add_node(Subtree{subobj, prop_node->id});
            connect_recursive(suboj_node, false);
        }

        connect_docnode(node, prop_node);
    }

    void on_connect(model::Object* object, Subtree* tree, bool insert_row, ReferencedPropertiesMap* referenced) override
    {
        model::VisualNode* visual = object->cast<model::VisualNode>();
        model::DocumentNode* node = nullptr;

        // connect visual node signals
        if ( visual )
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

            node = visual;
        }
        else
        {
            node = object->cast<model::DocumentNode>();
        }

        // connect document node signals
        if ( node )
        {
            connect(node, &model::DocumentNode::name_changed, model, [this, node]() {
                QModelIndex ind = node_index(node);
                QModelIndex par = node_index(node->docnode_parent());
                QModelIndex changed = model->index(ind.row(), ColumnName, par);
                model->dataChanged(changed, changed, {Qt::EditRole, Qt::DisplayRole});
            });
        }

        model::ObjectListPropertyBase* object_list = nullptr;

        for ( model::BaseProperty* prop : object->properties() )
        {
            if (
                (prop->traits().flags & model::PropertyTraits::List) &&
                prop->traits().type == model::PropertyTraits::Object
            )
            {
                // found "shapes"
                if ( node )
                    object_list = static_cast<model::ObjectListPropertyBase*>(prop);
            }
            // sub object
            else if ( prop->traits().type == model::PropertyTraits::Object )
            {
                model::Object* subobj = prop->value().value<model::Object*>();
                if ( subobj )
                {
                    // For assets, avoid an intermediate node
                    if ( object == document->assets() )
                    {
                        model::DocumentNode* subobj = prop->value().value<model::DocumentNode*>();
                        model::ObjectListPropertyBase* asset_list = static_cast<model::ObjectListPropertyBase*>(subobj->get_property("values"));
                        Subtree* prop_node = add_property(prop, tree->id, insert_row, referenced);
//                         Subtree* prop_node = add_node(Subtree{prop, tree->id});
//                         properties[asset_list] = prop_node->id;
                        on_connect_object_list(prop_node, subobj, asset_list);
                    }
                    else
                    {
                        auto meta = subobj->metaObject();
                        if (
                            !meta->inherits(&model::AnimationContainer::staticMetaObject) &&
                            !meta->inherits(&model::StretchableTime::staticMetaObject) &&
                            !meta->inherits(&model::MaskSettings::staticMetaObject)
                        )
                            connect_subobject(subobj, tree, insert_row);
                    }
                }
            }
            // sub object
            else if ( prop->traits().type == model::PropertyTraits::ObjectReference )
            {
                if ( prop->name() == "parent" )
                    continue;

                Subtree* prop_node = add_property(prop, tree->id, insert_row, referenced);

                if ( prop->name() != "composition" )
                {
                    model::Object* subobj = prop->value().value<model::Object*>();
                    connect_subobject(subobj, prop_node, insert_row);
                }
            }
            // scalar
            else if ( prop->traits().flags & model::PropertyTraits::Visual && prop->name() != "visible" )
            {
                add_property(prop, tree->id, insert_row, referenced);
            }
        }

        // Show object lists at the end
        if ( object_list )
        {
            tree->prop = object_list;
            on_connect_object_list(tree, node, object_list);
        }
    }

    void connect_docnode(model::DocumentNode* node, Subtree* insert_into)
    {
        auto id = insert_into->id;
        connect(node, &model::DocumentNode::docnode_child_add_end, model,
        [this, id, node](model::DocumentNode* child, int row) {
            auto insert_into = this->node(id);
            int rows = node->docnode_child_count() - 1; // called at the end
            add_object(child, insert_into, true, rows -  row + insert_into->merged_children_offset);
        });
        connect(node, &model::DocumentNode::docnode_child_remove_end, model,
        [this](model::DocumentNode* child) {
            on_delete_object(child);
        });
        connect(node, &model::DocumentNode::docnode_child_move_begin, model, [this, id, node](int a, int b) {
            auto insert_into = this->node(id);
            int rows = node->docnode_child_count();
            int src = rows - a - 1 + insert_into->merged_children_offset;
            int dest = rows - b - 1 + insert_into->merged_children_offset;
            int dest_it = dest;
            if ( src < dest )
                dest++;

            auto subtree = this->node(id);
            if ( !subtree )
                return;

            QModelIndex parent = subtree_index(subtree);
            my_model()->beginMoveRows(parent, src, src, parent, dest);

            Subtree* moved = subtree->children[src];
            subtree->children.erase(subtree->children.begin() + src);
            subtree->children.insert(subtree->children.begin() + dest_it, moved);

            my_model()->endMoveRows();
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

    if ( tree->visual_node && !tree->visual_node->docnode_locked_recursive() )
    {
        flags |= Qt::ItemIsDragEnabled;

        if ( tree->visual_node->has("shapes") )
            flags |= Qt::ItemIsDropEnabled;
    }

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
    {
        d->add_object(d->document->main(), nullptr, false);
        d->add_object(d->document->assets(), nullptr, false);
    }
}

int item_models::PropertyModelFull::columnCount(const QModelIndex &) const
{
    return ColumnCount;
}

std::pair<model::VisualNode *, int> item_models::PropertyModelFull::drop_position(const QModelIndex& parent, int row) const
{
    auto tree = d->node_from_index(parent);
    if ( !tree || !tree->visual_node )
        return {};

    row -= tree->merged_children_offset;

    if ( row < 0 )
        return {};

    return {tree->visual_node, row};
}
