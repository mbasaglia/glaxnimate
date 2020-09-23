#include "property_model.hpp"

#include <unordered_map>
#include <QFont>
#include <QPalette>

#include "command/property_commands.hpp"
#include "app/application.hpp"
#include "widgets/enum_combo.hpp"

class item_models::PropertyModel::Private
{
public:
    using id_type = quintptr;

    struct Subtree
    {
        Subtree(model::Object* object, id_type parent)
            : object{object}, parent{parent}
        {}

        Subtree(model::BaseProperty* prop, id_type parent)
            : prop{prop}, parent{parent}
        {}

        Subtree(int prop_index, id_type parent)
            : prop_index(prop_index), parent(parent)
        {}

        Subtree(model::Object* object, int prop_index, id_type parent)
            : object{object}, prop_index{prop_index}, parent{parent}
        {}

        model::Object* object = nullptr;
        model::BaseProperty* prop = nullptr;
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

    /*void connect_list(Subtree* prop_node)
    {
        if ( prop_node->prop->traits().is_object() )
        {
            for ( int i = 0; i < prop_node->prop_value.size(); i++ )
            {
                model::Object* subobj = prop_node->prop_value[i].value<model::Object*>();
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
    }*/

    void connect_recursive(model::Object* object, PropertyModel* model, id_type this_node)
    {
        if ( !object )
            return;

        objects[object] = this_node;
        QObject::connect(object, &model::Object::property_changed, model, &PropertyModel::property_changed);
        QObject::connect(object, &model::Object::destroyed, model, &PropertyModel::on_delete_object);

        for ( model::BaseProperty* prop : object->properties() )
        {
            if ( animation_only )
            {
                if ( prop->traits().type == model::PropertyTraits::Object )
                {
                    model::Object* subobj = prop->value().value<model::Object*>();
                    connect_recursive(subobj, model, this_node);
                }
                else if ( prop->traits().flags & model::PropertyTraits::Animated )
                {
                    properties[prop] = add_node(Subtree{prop, this_node});
                }
            }
            else
            {
                if (
                    (prop->traits().flags & model::PropertyTraits::List) &&
                    prop->traits().type == model::PropertyTraits::Object
                )
                    continue;

                id_type prop_node_id = add_node(Subtree{prop, this_node});
                Subtree* prop_node = node(prop_node_id);
                properties[prop] = prop_node_id;

                /*if ( prop->traits().flags & model::PropertyTraits::List )
                {
                    prop_node->prop_value = prop->value().toList();
                    connect_list(prop_node);
                }
                else*/ if ( prop->traits().is_object() )
                {
                    model::Object* subobj = prop->value().value<model::Object*>();
                    prop_node->object = subobj;
                    if ( prop->traits().type == model::PropertyTraits::Object )
                        connect_recursive(subobj, model, prop_node_id);
                }
            }
        }
    }

    void on_delete_object(model::Object* obj, PropertyModel* model)
    {
        auto it = objects.find(obj);
        if ( it == objects.end() )
            return;

        auto it2 = nodes.find(it->second);
        if ( it2 == nodes.end() )
            return;

        Subtree* node = &it2->second;
        for ( Subtree* child : node->children )
        {
            disconnect_recursive(child, model);
            nodes.erase(child->id);
        }

        if ( node->parent )
        {
            auto& siblings = this->node(node->parent)->children;
            for ( auto itc = siblings.begin(); itc != siblings.end(); ++itc )
            {
                if ( *itc == node )
                {
                    siblings.erase(itc);
                    break;
                }
            }
        }

        objects.erase(it);
        nodes.erase(it2);
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
        auto it = nodes.find(root_id);
        if ( it != nodes.end() )
            disconnect_recursive(&it->second, model);
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
        auto it = nodes.find(id);
        return it != nodes.end() ? &it->second : nullptr;
    }

    model::Document* document = nullptr;
    model::Object* root = nullptr;
    id_type root_id = 0;
    id_type next_id = 1;
    std::unordered_map<id_type, Subtree> nodes;
    std::unordered_map<model::Object*, id_type> objects;
    std::unordered_map<model::BaseProperty*, id_type> properties;
    bool animation_only;
};

item_models::PropertyModel::PropertyModel(bool animation_only)
    : d(std::make_unique<Private>())
{
    d->animation_only = animation_only;
}

item_models::PropertyModel::~PropertyModel() = default;


QModelIndex item_models::PropertyModel::index(int row, int column, const QModelIndex& parent) const
{
    Private::Subtree* tree = d->node_from_index(parent);
    if ( !tree )
        return {};


    if ( row >= 0 && row < int(tree->children.size()) )
        return createIndex(row, column, tree->children[row]->id);

    return {};
}

QModelIndex item_models::PropertyModel::parent(const QModelIndex& child) const
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

void item_models::PropertyModel::set_object(model::Object* object)
{
    if ( d->root )
    {
        d->disconnect(this);
    }

    beginResetModel();
    d->connect(object, this);
    endResetModel();
}

int item_models::PropertyModel::columnCount(const QModelIndex&) const
{
    return 2;
}

int item_models::PropertyModel::rowCount(const QModelIndex& parent) const
{
    if ( !d->root )
        return 0;

    Private::Subtree* tree = d->node_from_index(parent);
    if ( !tree )
        return 0;

    return tree->children.size();
}

Qt::ItemFlags item_models::PropertyModel::flags(const QModelIndex& index) const
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
            model::PropertyTraits traits = tree->prop->traits();

            if ( (traits.flags & (model::PropertyTraits::List|model::PropertyTraits::ReadOnly))
                || traits.type == model::PropertyTraits::Object || traits.type == model::PropertyTraits::Unknown )
                return flags;

            if ( traits.type == model::PropertyTraits::Bool )
                return flags | Qt::ItemIsUserCheckable;

            return flags | Qt::ItemIsEditable;
        }
        else
        {
            return Qt::ItemIsSelectable;
        }
    }

    return {};
}

QVariant item_models::PropertyModel::data(const QModelIndex& index, int role) const
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
        if ( !tree->prop )
        {
            if ( tree->object && role == Qt::DisplayRole )
                return tree->object->object_name();
            return {};
        }

        model::BaseProperty* prop = tree->prop;
        model::PropertyTraits traits = prop->traits();

        if ( role == Qt::ForegroundRole )
        {
            if ( (traits.flags & (model::PropertyTraits::List|model::PropertyTraits::ReadOnly))
                || traits.type == model::PropertyTraits::Object || traits.type == model::PropertyTraits::Unknown
            )
                return QApplication::palette().color(QPalette::Disabled, QPalette::Text);
        }

        if ( (traits.flags & model::PropertyTraits::Animated) )
        {
            model::AnimatableBase* anprop = static_cast<model::AnimatableBase*>(prop);
            auto frame_status = anprop->keyframe_status(d->document->current_time());

            if ( role == Qt::DecorationRole )
            {
                switch ( frame_status )
                {
                    case model::AnimatableBase::Tween:
                        return QIcon(app::Application::instance()->data_file("images/keyframe/status/tween.svg"));
                    case model::AnimatableBase::IsKeyframe:
                        return QIcon(app::Application::instance()->data_file("images/keyframe/status/key.svg"));
                    case model::AnimatableBase::Mismatch:
                        return QIcon(app::Application::instance()->data_file("images/keyframe/status/mismatch.svg"));
                    case model::AnimatableBase::NotAnimated:
                        return QIcon(app::Application::instance()->data_file("images/keyframe/status/not-animated.svg"));
                }

            }
            else if ( role == Qt::BackgroundRole )
            {
                switch ( frame_status )
                {
                    case model::AnimatableBase::Tween:
                        return QColor::fromHsv(100, 167, 127);
                    case model::AnimatableBase::IsKeyframe:
                        return QColor::fromHsv(51, 171, 133);
                    case model::AnimatableBase::Mismatch:
                        return QColor::fromHsv(29, 180, 149);
                    case model::AnimatableBase::NotAnimated:
                        return QColor::fromHsv(0, 0, 120);
                }
            }
            else if ( role == Qt::ForegroundRole )
            {
                return QColor(Qt::white);
            }
        }

        if ( (traits.flags & model::PropertyTraits::List) || traits.type == model::PropertyTraits::Unknown )
        {
            return {};
        }
        else if ( traits.type == model::PropertyTraits::Object )
        {
            if ( tree->object && role == Qt::DisplayRole )
                return tree->object->object_name();
            return {};
        }
        else if ( traits.type == model::PropertyTraits::Bool )
        {
            if ( role == Qt::CheckStateRole )
                return QVariant::fromValue(prop->value().toBool() ? Qt::Checked : Qt::Unchecked);
            return {};
        }
        else if ( traits.type == model::PropertyTraits::ObjectReference )
        {
            if ( role == Qt::DisplayRole )
            {
                QVariant value = prop->value();
                if ( value.isNull() )
                    return "";
                return value.value<model::DocumentNode*>()->object_name();
            }

            if ( role == Qt::DecorationRole )
            {
                QVariant value = prop->value();
                if ( value.isNull() )
                    return {};
                return QIcon(value.value<model::DocumentNode*>()->reftarget_icon());
            }

            if ( role == ReferenceProperty )
                return QVariant::fromValue(static_cast<model::ReferencePropertyBase*>(tree->prop));

            return {};
        }
        else if ( traits.type == model::PropertyTraits::Enum )
        {
            if ( role == Qt::DisplayRole )
                return EnumCombo::data_for(prop->value()).first;
            if ( role == Qt::EditRole )
                return prop->value();
            if ( role == Qt::DecorationRole )
                return QIcon::fromTheme(EnumCombo::data_for(prop->value()).second);
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

bool item_models::PropertyModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if ( !d->root || !index.isValid() || index.column() != 1 )
        return false;

    Private::Subtree* tree = d->node_from_index(index);
    if ( !tree )
        return false;

    if ( !tree->prop )
        return false;

    model::BaseProperty* prop = tree->prop;
    model::PropertyTraits traits = prop->traits();


    if ( (traits.flags & (model::PropertyTraits::List|model::PropertyTraits::ReadOnly)) ||
        traits.type == model::PropertyTraits::Object ||
        traits.type == model::PropertyTraits::Unknown )
    {
        return false;
    }
    else if ( traits.type == model::PropertyTraits::Bool )
    {
        if ( role == Qt::CheckStateRole )
        {
            return prop->set_undoable(value.value<Qt::CheckState>() == Qt::Checked);
        }
        return false;
    }
    else
    {
        if ( role == Qt::EditRole )
        {
            return prop->set_undoable(value);
        }
        return false;
    }
}

void item_models::PropertyModel::property_changed(const model::BaseProperty* prop, const QVariant& value)
{
    auto it = d->properties.find(const_cast<model::BaseProperty*>(prop));
    if ( it == d->properties.end() )
        return;

    Private::Subtree* prop_node = d->node(it->second);
    if ( !prop_node )
        return;

    Private::Subtree* parent = d->node(prop_node->parent);

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
        else if ( prop_node->prop->traits().type == model::PropertyTraits::ObjectReference )
        {
            beginRemoveRows(index, 0, prop_node->children.size());
            d->disconnect_recursive(prop_node, this);
            endRemoveRows();
            model::Object* object = value.value<model::Object*>();
            beginInsertRows(index, 0, prop_node->object->properties().size());
            prop_node->object = object;
            d->connect_recursive(object, this, prop_node->id);
            endInsertRows();
        }

        dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
    }

}

QVariant item_models::PropertyModel::headerData(int section, Qt::Orientation orientation, int role) const
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

void item_models::PropertyModel::set_document(model::Document* document)
{
    d->document = document;
    clear_object();
}

model::AnimatableBase * item_models::PropertyModel::animatable(const QModelIndex& index) const
{
    if ( Private::Subtree* st = d->node_from_index(index) )
    {
        if ( st->prop && st->prop->traits().flags & model::PropertyTraits::Animated )
            return static_cast<model::AnimatableBase*>(st->prop);
    }

    return nullptr;
}

void item_models::PropertyModel::on_delete_object()
{
    d->on_delete_object(static_cast<model::Object*>(sender()), this);
}

QModelIndex item_models::PropertyModel::property_index(model::BaseProperty* prop) const
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
