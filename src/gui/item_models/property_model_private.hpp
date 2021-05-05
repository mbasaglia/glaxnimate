#pragma once
#include "property_model_base.hpp"

#include <unordered_map>

#include <QFont>
#include <QPalette>

#include "model/property/option_list_property.hpp"

#include "app/application.hpp"

#include "widgets/enum_combo.hpp"


class item_models::PropertyModelBase::Private
{
public:
    using id_type = quintptr;

    struct Subtree
    {
        Subtree(model::Object* object, id_type parent)
            : object{object}, parent{parent}, visual_node{object->cast<model::VisualNode>()}
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

        int child_index(Subtree* child) const
        {
            for ( int i = 0; i < int(children.size()); i++ )
                if ( children[i] == child )
                    return i;
            return -1;
        }

        model::Object* object = nullptr;
        model::BaseProperty* prop = nullptr;
        QVariantList prop_value;
        int prop_index = -1;
        id_type parent = 0;
        std::vector<Subtree*> children;
        id_type id = 0;
        model::VisualNode* visual_node = nullptr;
    };

    explicit Private(PropertyModelBase* parent)
        : model(parent)
    {
    }

    virtual ~Private() {}


    void add_object(model::Object* object, Subtree* parent, bool insert_row)
    {
        auto& container = parent ? parent->children : roots;
        if ( std::find_if(container.begin(), container.end(), [object](Subtree* st){ return st->object == object; }) != container.end() )
            return;

        if ( insert_row )
            model->beginInsertRows(subtree_index(parent), container.size(), container.size());

        auto node = add_node(Subtree{object, 0});
        roots.push_back(node);
        connect_recursive(node);

        if ( insert_row )
            model->endInsertRows();

//         add_extra_objects(object, parent, insert_row);
    }

    Subtree* add_node(Subtree st)
    {
        auto it = nodes.insert({next_id, st}).first;
        if ( st.parent )
            node(st.parent)->children.push_back(&it->second);
        it->second.id = next_id;
        next_id++;
        return &it->second;
    }

    void clear()
    {
        for ( const auto& p : roots )
            disconnect_recursive(p);

        roots.clear();
        next_id = 1;
        nodes.clear();
        objects.clear();
    }

    Subtree* node_from_index(const QModelIndex& index)
    {
        if ( !index.isValid() )
            return nullptr;

        auto it = nodes.find(index.internalId());
        return it == nodes.end() ? nullptr : &it->second;
    }

    Subtree* node(id_type id)
    {
        auto it = nodes.find(id);
        return it != nodes.end() ? &it->second : nullptr;
    }

    QVariant data_name(Subtree* tree, int role)
    {
        if ( role == Qt::DisplayRole )
        {
            if ( tree->prop_index != -1 )
                return tree->prop_index;
            else if ( tree->prop )
                return tree->prop->name();
            else if ( tree->object )
                return tree->object->object_name();
        }
        else if ( role == Qt::FontRole )
        {
            QFont font;
            font.setBold(true);
            return font;
        }
        else if ( role == Qt::EditRole && tree->visual_node )
        {
            return tree->visual_node->object_name();
        }
        else if ( role == Qt::DecorationRole && tree->visual_node )
        {
            return tree->visual_node->tree_icon();
        }


        return {};
    }

    QVariant data_value(Subtree* tree, int role)
    {
        if ( !tree->prop )
        {
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

        if ( role == Flags )
            return tree->prop->traits().flags;

        if ( role == ReferenceProperty && tree->prop->traits().flags & model::PropertyTraits::OptionList )
            return QVariant::fromValue(static_cast<model::OptionListPropertyBase*>(tree->prop));

        if ( (traits.flags & model::PropertyTraits::Animated) )
        {
            model::AnimatableBase* anprop = static_cast<model::AnimatableBase*>(prop);
            auto frame_status = anprop->keyframe_status(document->current_time());

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
            else if ( role == MinValue && traits.type == model::PropertyTraits::Float )
            {
                return static_cast<model::AnimatedProperty<float>*>(anprop)->min();
            }
            else if ( role == MaxValue && traits.type == model::PropertyTraits::Float )
            {
                return static_cast<model::AnimatedProperty<float>*>(anprop)->max();
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
                return QIcon(value.value<model::DocumentNode*>()->instance_icon());
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
            if ( role == Qt::DisplayRole && (prop->traits().flags & model::PropertyTraits::Percent) )
                return QString(tr("%1%").arg(prop->value().toDouble() * 100));
            if ( role == Qt::DisplayRole || role == Qt::EditRole )
                return prop->value();
            return {};
        }
    }

    virtual void on_connect(model::Object* object, Subtree* tree) = 0;

    void connect_recursive(Subtree* this_node)
    {
        auto object = this_node->object;
        if ( !object )
            return;

        objects[object] = this_node->id;
        QObject::connect(object, &model::Object::destroyed, model, &PropertyModelBase::on_delete_object);
        QObject::connect(object, &model::Object::removed_from_list, model, &PropertyModelBase::on_delete_object);
        QObject::connect(object, &model::Object::property_changed, model, &PropertyModelBase::property_changed);

        on_connect(object, this_node);
    }

    void connect_subobject(model::Object* object, Subtree* this_node)
    {
        if ( !object )
            return;

        QObject::connect(object, &model::Object::property_changed, model, &PropertyModelBase::property_changed);;
        on_connect(object, this_node);
    }


    void disconnect_recursive(Subtree* node)
    {
        if ( node->object )
        {
            QObject::disconnect(node->object, nullptr, model, nullptr);
            objects.erase(node->object);
        }

        for ( Subtree* child : node->children )
        {
            disconnect_recursive(child);
            nodes.erase(child->id);
        }

        node->children.clear();
    }

    Subtree* object_tree(model::Object* obj)
    {
        auto it1 = objects.find(obj);
        if ( it1 == objects.end() )
            return nullptr;

        auto it2 = nodes.find(it1->second);
        if ( it2 == nodes.end() )
            return nullptr;

        return &it2->second;
    }

    void on_delete_object(model::Object* obj)
    {
        auto it = objects.find(obj);
        if ( it == objects.end() )
            return;

        auto it2 = nodes.find(it->second);
        if ( it2 == nodes.end() )
            return;

        Subtree* node = &it2->second;

        auto index = model->object_index(obj);
        model->beginRemoveRows(index.parent(), index.row(), index.row());

        for ( Subtree* child : node->children )
        {
            disconnect_recursive(child);
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

        auto it_roots = std::find(roots.begin(), roots.end(), node);
        if ( it_roots != roots.end() )
            roots.erase(it_roots);

        objects.erase(it);
        nodes.erase(it2);

        model->endRemoveRows();

//         remove_extra_objects(obj, model);
    }

    Subtree* visual_node_parent(Subtree* tree)
    {
        while ( tree->parent )
        {
            tree = node(tree->parent);
            if ( !tree )
                return nullptr;

            if ( tree->visual_node )
                return tree;
        }

        return nullptr;
    }

    QModelIndex node_index(model::DocumentNode* node)
    {
        auto it = objects.find(node);
        if ( it == objects.end() )
            return {};

        Subtree* tree = this->node(it->second);
        if ( !tree )
            return {};

        int row = 0;
        if ( Subtree* parent = visual_node_parent(tree) )
        {
            row = parent->visual_node->docnode_child_index(node);
            if ( row == -1 )
                return {};
        }
        else
        {
            for ( ; row < int(roots.size()); row++ )
                if ( roots[row]->visual_node == node )
                    break;

            if ( row == int(roots.size()) )
                return {};
        }

        return model->createIndex(row, 0, tree->id);
    }

    QModelIndex subtree_index(id_type id)
    {
        auto it = nodes.find(id);
        if ( it == nodes.end() )
            return {};
        return subtree_index(&it->second);
    }

    QModelIndex subtree_index(Subtree* tree)
    {
        if ( !tree )
            return {};

        int row = 0;
        if ( tree->parent )
        {
            auto it = nodes.find(tree->parent);
            if ( it == nodes.end() )
                return {};

            auto parent = &*it;
            if ( !parent )
                return {};

            row = parent->second.child_index(tree);
            if ( row == -1 )
                return {};
        }
        else
        {
            for ( ; row < int(roots.size()); row++ )
                if ( roots[row] == tree )
                    break;

            if ( row == int(roots.size()) )
                return {};
        }

        return model->createIndex(row, 0, tree->id);
    }

    bool set_prop_data(Subtree* tree, const QVariant& value, int role)
    {
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
                return prop->set_undoable(value);
            return false;
        }
    }


    model::Document* document = nullptr;
    std::vector<Subtree*> roots;
    id_type next_id = 1;
    std::unordered_map<id_type, Subtree> nodes;
    std::unordered_map<model::Object*, id_type> objects;
    std::unordered_map<model::BaseProperty*, id_type> properties;
    bool animation_only;
    PropertyModelBase* model = nullptr;
};
