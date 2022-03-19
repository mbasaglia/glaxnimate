#pragma once
#include "property_model_base.hpp"

#include <unordered_map>

#include <QFont>
#include <QPalette>

#include "glaxnimate/core/model/property/option_list_property.hpp"

#include "app/application.hpp"

#include "widgets/enum_combo.hpp"


class glaxnimate::gui::item_models::PropertyModelBase::Private
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

        int child_index(Subtree* child) const
        {
            for ( int i = 0; i < int(children.size()); i++ )
                if ( children[i] == child )
                    return i;
            return -1;
        }

        model::Object* object = nullptr;
        model::BaseProperty* prop = nullptr;
        id_type parent = 0;
        std::vector<Subtree*> children;
        id_type id = 0;
        model::VisualNode* visual_node = nullptr;
        bool expand_referenced = false;
        /// When showing children directly, offset by this much
        int merged_children_offset = 0;
    };

    using ReferencedPropertiesMap = std::unordered_map<model::BaseProperty*, std::vector<id_type>>;

    explicit Private(PropertyModelBase* parent)
        : model(parent)
    {
    }

    virtual ~Private() {}


    void add_object(model::Object* object, Subtree* parent, bool insert_row, int index = -1);

    Subtree* do_add_node(Subtree st, Subtree* parent, int index);

    Subtree* add_node(Subtree st);

    void clear();

    Subtree* node_from_index(const QModelIndex& index);

    Subtree* node(id_type id);

    QVariant data_name(Subtree* tree, int role);

    QVariant data_value(Subtree* tree, int role);
    QVariant data_value(model::BaseProperty* prop, model::Object* object, int role);

    virtual void on_connect(model::Object* object, Subtree* tree, bool insert_row, ReferencedPropertiesMap* referenced) = 0;

    void connect_recursive(Subtree* this_node, bool insert_row);

    void connect_subobject(model::Object* object, Subtree* this_node, bool insert_row);

    void disconnect_recursive(Subtree* node);

    Subtree* object_tree(model::Object* obj);

    void on_delete_object(model::Object* obj);

    Subtree* visual_node_parent(Subtree* tree);

    QModelIndex node_index(model::DocumentNode* node);

    QModelIndex subtree_index(id_type id);

    QModelIndex subtree_index(Subtree* tree);

    bool set_prop_data(Subtree* tree, const QVariant& value, int role);
    bool set_prop_data(model::BaseProperty* prop, const QVariant& value, int role);

    void property_changed(const model::BaseProperty* prop, const QVariant& value);
    void on_property_changed(id_type prop_node_id, const model::BaseProperty* prop, const QVariant& value);
    void clean_object_references(const QModelIndex& index, Private::Subtree* prop_node);

    void begin_insert_row(Subtree* row_tree, int index);
    void end_insert_row();
    Subtree* add_property(model::BaseProperty* prop, id_type parent, bool insert_row, ReferencedPropertiesMap* referenced);

    void clean_subtree(Subtree* node);

    model::Document* document = nullptr;
    std::vector<Subtree*> roots;
    id_type next_id = 1;
    std::unordered_map<id_type, Subtree> nodes;
    std::unordered_map<model::Object*, id_type> objects;
    std::unordered_map<model::BaseProperty*, id_type> properties;
    std::unordered_map<model::Object*, ReferencedPropertiesMap> referenced_properties;
    bool animation_only;
    PropertyModelBase* model = nullptr;
};
