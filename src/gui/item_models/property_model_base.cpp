#include "property_model_private.hpp"


item_models::PropertyModelBase::PropertyModelBase(std::unique_ptr<Private> d)
    : d(std::move(d))
{
}

item_models::PropertyModelBase::~PropertyModelBase()
{
}

QModelIndex item_models::PropertyModelBase::index(int row, int column, const QModelIndex& parent) const
{
    Private::Subtree* tree = d->node_from_index(parent);
    if ( !tree )
    {
        if ( row >= 0 && row < int(d->roots.size()) )
            return createIndex(row, column, d->roots[row]->id);
        return {};
    }


    if ( row >= 0 && row < int(tree->children.size()) )
        return createIndex(row, column, tree->children[row]->id);

    return {};
}

QModelIndex item_models::PropertyModelBase::parent(const QModelIndex& child) const
{
    Private::Subtree* tree = d->node_from_index(child);

    if ( !tree || tree->parent == 0 )
        return {};

    auto it = d->nodes.find(tree->parent);
    if ( it == d->nodes.end() )
        return {};

    Private::Subtree* tree_parent = &it->second;

    for ( int i = 0; i < int(tree_parent->children.size()); i++ )
        if ( tree_parent->children[i] == tree )
            return createIndex(i, 0, tree->parent);

    return {};
}

void item_models::PropertyModelBase::set_document(model::Document* document)
{
    beginResetModel();
    d->document = document;
    d->clear();
    on_document_reset();
    endResetModel();
}

void item_models::PropertyModelBase::clear_document()
{
    set_document(nullptr);
}

item_models::PropertyModelBase::Item item_models::PropertyModelBase::item(const QModelIndex& index) const
{
    if ( Private::Subtree* st = d->node_from_index(index) )
    {
        Item item = st->object;
        if ( st->prop )
            item.property = st->prop;
        return item;
    }

    return {};
}


QModelIndex item_models::PropertyModelBase::property_index(model::BaseProperty* prop) const
{
    auto it = d->properties.find(prop);
    if ( it == d->properties.end() )
        return {};

    Private::Subtree* prop_node = d->node(it->second);
    if ( !prop_node )
        return {};

    Private::Subtree* parent = d->node(prop_node->parent);

    int i = std::find(parent->children.begin(), parent->children.end(), prop_node) - parent->children.begin();
    return createIndex(i, 1, prop_node->id);
}

QModelIndex item_models::PropertyModelBase::object_index(model::Object* obj) const
{
    auto it = d->objects.find(obj);
    if ( it == d->objects.end() )
        return {};

    Private::Subtree* prop_node = d->node(it->second);
    if ( !prop_node )
        return {};

    Private::Subtree* parent = d->node(prop_node->parent);

    int i = 0;
    if ( !parent )
        i = std::find(d->roots.begin(), d->roots.end(), prop_node) - d->roots.begin();
    else
        i = std::find(parent->children.begin(), parent->children.end(), prop_node) - parent->children.begin();

    return createIndex(i, 1, prop_node->id);
}

model::VisualNode* item_models::PropertyModelBase::visual_node(const QModelIndex& index) const
{
    Private::Subtree* tree = d->node_from_index(index);
    if ( !tree )
        return nullptr;

    return tree->visual_node;
}

void item_models::PropertyModelBase::on_delete_object()
{
    d->on_delete_object(static_cast<model::Object*>(sender()));
}


void item_models::PropertyModelBase::property_changed(const model::BaseProperty* prop, const QVariant& value)
{
    auto it = d->properties.find(const_cast<model::BaseProperty*>(prop));
    if ( it == d->properties.end() )
        return;

    Private::Subtree* prop_node = d->node(it->second);
    if ( !prop_node )
        return;

    Private::Subtree* parent = d->node(prop_node->parent);

    if ( !parent )
        return;

    int i = std::find(parent->children.begin(), parent->children.end(), prop_node) - parent->children.begin();
    QModelIndex index = createIndex(i, 1, prop_node->id);

    if ( prop_node->prop->traits().flags & model::PropertyTraits::List )
    {
        /*beginRemoveRows(index, 0, prop_node->children.size());
        d->disconnect_recursive(prop_node, this);
        endRemoveRows();


        beginInsertRows(index, 0, prop_node->prop_value.size());
        prop_node->prop_value = value.toList();
        d->connect_list(prop_node);
        endInsertRows();*/
    }
    else
    {
        if ( prop_node->prop->traits().type == model::PropertyTraits::ObjectReference )
        {
            prop_node->object = value.value<model::Object*>();
        }
        /*else if ( prop_node->prop->traits().type == model::PropertyTraits::ObjectReference )
        {
            beginRemoveRows(index, 0, prop_node->children.size());
            d->disconnect_recursive(prop_node);
            endRemoveRows();
            model::Object* object = value.value<model::Object*>();
            beginInsertRows(index, 0, prop_node->object->properties().size());
            prop_node->object = object;
            d->connect_recursive(object, this, prop_node->id);
            endInsertRows();
        }*/

        dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
    }
}


int item_models::PropertyModelBase::rowCount(const QModelIndex& parent) const
{
    if ( d->roots.empty() )
        return 0;

    Private::Subtree* tree = d->node_from_index(parent);
    if ( !tree )
        return d->roots.size();

    return tree->children.size();
}

