#include "property_model_single.hpp"
#include "property_model_private.hpp"


#include "command/property_commands.hpp"
#include "model/shapes/styler.hpp"
#include "model/stretchable_time.hpp"


class item_models::PropertyModelSingle::Private : public PropertyModelBase::Private
{
public:
    using PropertyModelBase::Private::Private;
    /*
    void add_extra_objects(model::Object* object, PropertyModelSingle* model, bool insert_row)
    {
        auto mo = object->metaObject();
        if ( mo->inherits(&model::Styler::staticMetaObject) )
        {
            auto styler = static_cast<model::Styler*>(object);
            if ( styler->use.get() )
                model->add_object(styler->use.get());
            QObject::connect(styler, &model::Styler::use_changed_from, model,
            [this, model, insert_row](model::BrushStyle* old_use, model::BrushStyle* new_use){
                if ( old_use )
                    on_delete_object(old_use, model);
                if ( new_use )
                    add_object(new_use, model, insert_row);
            });
        }
        else if ( mo->inherits(&model::Gradient::staticMetaObject) )
        {
            auto gradient = static_cast<model::Gradient*>(object);
            if ( gradient->colors.get() )
                model->add_object(gradient->colors.get());
            QObject::connect(gradient, &model::Gradient::colors_changed_from, model,
            [this, model, insert_row](model::GradientColors* old_use, model::GradientColors* new_use){
                if ( old_use )
                    on_delete_object(old_use, model);
                if ( new_use )
                    add_object(new_use, model, insert_row);
            });
        }
    }

    void remove_extra_objects(model::Object* object, PropertyModelSingle* model)
    {
        auto mo = object->metaObject();
        if ( mo->inherits(&model::Styler::staticMetaObject) )
        {
            auto styler = static_cast<model::Styler*>(object);
            if ( styler->use.get() )
                on_delete_object(styler->use.get(), model);
        }
        else if ( mo->inherits(&model::Gradient::staticMetaObject) )
        {
            auto gradient = static_cast<model::Gradient*>(object);
            if ( gradient->colors.get() )
                on_delete_object(gradient->colors.get(), model);
        }
    }

    void add_object_without_properties(model::Object* object, PropertyModelSingle* model)
    {
        auto node = add_node(Subtree{object, 0});
        roots.push_back(node);
        emit model->root_object_added_begin(object);
        QObject::connect(object, &model::Object::destroyed, model, &PropertyModelSingle::on_delete_object);
    }*/


    void on_connect(model::Object* object, Subtree* this_node) override
    {
        for ( model::BaseProperty* prop : object->properties() )
        {

            if (
                (prop->traits().flags & model::PropertyTraits::List) &&
                prop->traits().type == model::PropertyTraits::Object
            )
            {
                continue;
            }

            Subtree* prop_node = add_node(Subtree{prop, this_node->id});
            properties[prop] = prop_node->id;

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
                    connect_subobject(subobj, prop_node);
            }
        }
    }
};

item_models::PropertyModelSingle::PropertyModelSingle()
    : PropertyModelBase(std::make_unique<Private>(this))
{
}

void item_models::PropertyModelSingle::set_object(model::Object* object)
{
    beginResetModel();
    d->clear();
    if ( object )
        d->add_object(object, nullptr, false);
    endResetModel();
}
/*
void item_models::PropertyModelSingle::add_object(model::Object* object)
{
    d->add_object(object, this, true);
}

void item_models::PropertyModelSingle::add_object_without_properties(model::Object* object)
{
    beginInsertRows({}, d->roots.size(), d->roots.size());
    d->add_object_without_properties(object, this);
    endInsertRows();
}*/

int item_models::PropertyModelSingle::columnCount(const QModelIndex&) const
{
    return ColumnCount;
}

Qt::ItemFlags item_models::PropertyModelSingle::flags(const QModelIndex& index) const
{
    if ( d->roots.empty() || !index.isValid() )
        return {};

    Private::Subtree* tree = d->node_from_index(index);
    if ( !tree )
        return {};

    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    switch ( index.column() )
    {
        case ColumnName:
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

QVariant item_models::PropertyModelSingle::data(const QModelIndex& index, int role) const
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
    }
    return {};
}

bool item_models::PropertyModelSingle::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if ( d->roots.empty() || !index.isValid() )
        return false;

    Private::Subtree* tree = d->node_from_index(index);
    if ( !tree )
        return false;

    if ( index.column() == ColumnValue )
        return d->set_prop_data(tree, value, role);

    return false;
}

QVariant item_models::PropertyModelSingle::headerData(int section, Qt::Orientation orientation, int role) const
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
        }
    }
    return {};
}
