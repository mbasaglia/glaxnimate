#include "property_model.hpp"

#include <unordered_map>
#include <QFont>

#include "command/property_commands.hpp"

class model::PropertyModel::Private
{
public:
    using id_type = quintptr;

    struct Subtree
    {
        Subtree(Object* object, id_type parent)
            : object{object}, parent{parent}
        {}

        Subtree(BaseProperty* prop, id_type parent)
            : prop{prop}, parent{parent}
        {}

        Subtree(int prop_index, id_type parent)
            : prop_index(prop_index), parent(parent)
        {}

        Subtree(Object* object, int prop_index, id_type parent)
            : object{object}, prop_index{prop_index}, parent{parent}
        {}

        Object* object = nullptr;
        BaseProperty* prop = nullptr;
        QVariantList prop_value;
        int prop_index = -1;
        id_type parent = 0;
        std::vector<Subtree*> children;
        id_type id = 0;

    };

    void connect(model::Object* object, PropertyModel* model)
    {
        root = object;
        root_id = next_id;
        add_node(Subtree{object, 0});
        connect_recursive(object, model, root_id);
    }

    void connect_list(Subtree* prop_node)
    {
        if ( prop_node->prop->traits().is_object() )
        {
            for ( int i = 0; i < prop_node->prop_value.size(); i++ )
            {
                Object* subobj = prop_node->prop_value[i].value<Object*>();
                add_node(Subtree{subobj, i, prop_node->id});
                // connect_recursive(subobj, model, subobj_id);
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

    void connect_recursive(model::Object* object, PropertyModel* model, id_type this_node)
    {
        if ( !object )
            return;

        objects[object] = this_node;
        QObject::connect(object, &model::Object::property_changed, model, &PropertyModel::property_changed);

        for ( BaseProperty* prop : object->properties() )
        {
            id_type prop_node_id = add_node(Subtree{prop, this_node});
            Subtree* prop_node = node(prop_node_id);

            if ( prop->traits().list )
            {
                prop_node->prop_value = prop->value().toList();
                connect_list(prop_node);
            }
            else if ( prop->traits().is_object() )
            {
                Object* subobj = prop->value().value<Object*>();
                prop_node->object = subobj;
                if ( prop->traits().type == PropertyTraits::Object )
                    connect_recursive(subobj, model, prop_node_id);
            }
        }
    }

    void disconnect_recursive(Subtree* node, PropertyModel* model)
    {
        if ( node->object )
        {
            QObject::disconnect(node->object, nullptr, model, nullptr);
            objects.erase(node->object);
        }
        for ( Subtree* child : node->children )
        {
            disconnect_recursive(child, model);
            nodes.erase(child->id);
        }
        node->children.clear();
    }

    void disconnect(PropertyModel* model)
    {
        disconnect_recursive(node(root_id), model);
        root = nullptr;
        nodes.clear();
        objects.clear();
    }

    id_type add_node(Subtree st)
    {
        auto it = nodes.insert({next_id, st}).first;
        if ( st.parent )
            node(st.parent)->children.push_back(&it->second);
        it->second.id = next_id;
        return next_id++;
    }

    Subtree* node_from_index(const QModelIndex& index)
    {
        if ( !index.isValid() )
        {
            auto it = nodes.find(root_id);
            return it == nodes.end() ? nullptr : &it->second;
        }

        auto it = nodes.find(index.internalId());
        return it == nodes.end() ? nullptr : &it->second;
    }

    Subtree* node(id_type id)
    {
        return &nodes.find(id)->second;
    }

    bool set_property(model::BaseProperty* prop, const QVariant& after)
    {
        QVariant before = prop->value();
        if ( !prop->set_value(after) )
            return false;
        document->undo_stack().push(new command::SetPropertyValue(prop, before, after));
        return true;
    }


    model::Document* document = nullptr;
    Object* root = nullptr;
    id_type root_id = 0;
    id_type next_id = 1;
    std::unordered_map<id_type, Subtree> nodes;
    std::unordered_map<Object*, id_type> objects;
};

model::PropertyModel::PropertyModel()
    : d(std::make_unique<Private>())
{
}

model::PropertyModel::~PropertyModel() = default;


QModelIndex model::PropertyModel::index(int row, int column, const QModelIndex& parent) const
{
    Private::Subtree* tree = d->node_from_index(parent);
    if ( !tree )
        return {};


    if ( row >= 0 && row < int(tree->children.size()) )
        return createIndex(row, column, tree->children[row]->id);

    return {};
}

QModelIndex model::PropertyModel::parent(const QModelIndex& child) const
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

void model::PropertyModel::set_object(model::Object* object)
{
    if ( d->root )
    {
        d->disconnect(this);
    }

    beginResetModel();
    d->connect(object, this);
    endResetModel();
}

int model::PropertyModel::columnCount(const QModelIndex&) const
{
    return 2;
}

int model::PropertyModel::rowCount(const QModelIndex& parent) const
{
    if ( !d->root )
        return 0;

    Private::Subtree* tree = d->node_from_index(parent);
    if ( !tree )
        return 0;

    return tree->children.size();
}

Qt::ItemFlags model::PropertyModel::flags(const QModelIndex& index) const
{
    if ( !d->root || !index.isValid() )
        return {};

    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    if ( index.column() == 0 )
        return flags;


    Private::Subtree* tree = d->node_from_index(index);
    if ( !tree )
        return {};

    if ( index.column() == 1 )
    {
        if ( tree->prop )
        {
            PropertyTraits traits = tree->prop->traits();

            if ( traits.list || traits.type == PropertyTraits::Object || traits.type == PropertyTraits::Unknown || !traits.user_editable )
                return Qt::ItemIsSelectable;

            if ( traits.user_editable )
            {
                if ( traits.type == PropertyTraits::Bool )
                    return flags | Qt::ItemIsUserCheckable;

                return flags | Qt::ItemIsEditable;
            }
        }
        else
        {
            return Qt::ItemIsSelectable;
        }
    }

    return {};
}

QVariant model::PropertyModel::data(const QModelIndex& index, int role) const
{
    if ( !d->root || !index.isValid() )
        return {};


    Private::Subtree* tree = d->node_from_index(index);
    if ( !tree )
        return {};

    if ( index.column() == 0 )
    {
        if ( role == Qt::DisplayRole )
        {
            if ( tree->prop_index != -1 )
                return tree->prop_index;
            else if ( tree->prop )
                return tree->prop->name();
        }
        else if ( role == Qt::FontRole )
        {
            QFont font;
            font.setBold(true);
            return font;
        }
        return {};
    }
    else if ( index.column() == 1 )
    {

        BaseProperty* prop = tree->prop;
        PropertyTraits traits = prop->traits();

        if ( traits.list || traits.type == PropertyTraits::Unknown )
        {
            return {};
        }
        else if ( traits.type == PropertyTraits::Object )
        {
            if ( tree->object && role == Qt::DisplayRole )
                return tree->object->object_name();
            return {};
        }
        else if ( traits.type == PropertyTraits::Bool )
        {
            if ( role == Qt::CheckStateRole )
                return QVariant::fromValue(prop->value().toBool() ? Qt::Checked : Qt::Unchecked);
            return {};
        }
        else if ( traits.type == PropertyTraits::Enum )
        {
            if ( role == Qt::DisplayRole )
                return prop->value().toString();
            return {};
        }
        else if ( traits.type == PropertyTraits::ObjectReference )
        {
            if ( role == Qt::DisplayRole )
            {
                QVariant value = prop->value();
                if ( value.isNull() )
                    return "";
                return value.value<DocumentNode*>()->docnode_name();
            }

            if ( role == Qt::DecorationRole )
            {
                QVariant value = prop->value();
                if ( value.isNull() )
                    return {};
                return QIcon(value.value<DocumentNode*>()->docnode_group_icon());
            }

            if ( role == ReferenceProperty )
                return QVariant::fromValue(static_cast<ReferencePropertyBase*>(tree->prop));

            return {};
        }
        else
        {
            if ( role == Qt::DisplayRole || role == Qt::EditRole )
                return prop->value();
            return {};
        }
    }
    return {};
}

bool model::PropertyModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if ( !d->root || !index.isValid() || index.column() != 1 )
        return false;

    Private::Subtree* tree = d->node_from_index(index);
    if ( !tree )
        return false;

    if ( !tree->prop )
        return false;

    BaseProperty* prop = tree->prop;
    PropertyTraits traits = prop->traits();


    if ( traits.list || traits.type == PropertyTraits::Object ||
         traits.type == PropertyTraits::Unknown || !traits.user_editable )
    {
        return false;
    }
    else if ( traits.type == PropertyTraits::Bool )
    {
        if ( role == Qt::CheckStateRole )
        {
            return d->set_property(prop, value.value<Qt::CheckState>() == Qt::Checked);
        }
        return false;
    }
    else
    {
        if ( role == Qt::EditRole )
        {
            return d->set_property(prop, value);
        }
        return false;
    }
}

void model::PropertyModel::property_changed(const QString& name, const QVariant& value)
{
    auto it = d->objects.find((Object*)QObject::sender());
    if ( it == d->objects.end() )
        return;

    Private::Subtree* parent = d->node(it->second);
    if ( !parent )
        return;

    for ( int i = 0; i < int(parent->object->properties().size()); i++ )
    {
        if ( parent->object->properties()[i]->name() == name )
        {
            Private::Subtree* prop_node = parent->children[i];
            QModelIndex index = createIndex(i, 1, prop_node->id);

            if ( prop_node->prop->traits().list )
            {
                beginRemoveRows(index, 0, prop_node->children.size());
                d->disconnect_recursive(prop_node, this);
                endRemoveRows();


                beginInsertRows(index, 0, prop_node->prop_value.size());
                prop_node->prop_value = value.toList();
                d->connect_list(prop_node);
                endInsertRows();
            }
            else
            {
                if ( prop_node->prop->traits().type == PropertyTraits::ObjectReference )
                {
                    prop_node->object = value.value<Object*>();
                }
                else if ( prop_node->prop->traits().type == PropertyTraits::ObjectReference )
                {
                    beginRemoveRows(index, 0, prop_node->children.size());
                    d->disconnect_recursive(prop_node, this);
                    endRemoveRows();
                    Object* object = value.value<Object*>();
                    beginInsertRows(index, 0, prop_node->object->properties().size());
                    prop_node->object = object;
                    d->connect_recursive(object, this, prop_node->id);
                    endInsertRows();
                }

                dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
            }
            return;
        }
    }

}

QVariant model::PropertyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( orientation == Qt::Horizontal && role == Qt::DisplayRole )
    {
        if ( section == 0 )
            return tr("Name");
        if ( section == 1 )
            return tr("Value");
    }
    return {};
}

void model::PropertyModel::set_document(model::Document* document)
{
    d->document = document;
    clear_object();
}
