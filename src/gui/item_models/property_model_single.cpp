#include "property_model_single.hpp"
#include "property_model_private.hpp"


#include "command/property_commands.hpp"
#include "model/shapes/styler.hpp"
#include "model/stretchable_time.hpp"

using namespace glaxnimate::gui;
using namespace glaxnimate;


class item_models::PropertyModelSingle::Private : public PropertyModelBase::Private
{
public:
    using PropertyModelBase::Private::Private;

    void on_connect(model::Object* object, Subtree* this_node, bool insert_row, ReferencedPropertiesMap* referenced) override
    {
        for ( model::BaseProperty* prop : object->properties() )
        {

            // skip object lists
            if (
                (prop->traits().flags & model::PropertyTraits::List) &&
                prop->traits().type == model::PropertyTraits::Object
            )
                continue;

            // add the property node to the internal structures
            Subtree* prop_node = add_property(prop, this_node->id, insert_row, referenced);

            // connect references / sub-objects
            if ( prop->traits().is_object() )
            {
                model::Object* subobj = prop->value().value<model::Object*>();

                if ( prop->name() != "parent" && prop->name() != "composition" )
                    connect_subobject(subobj, prop_node, insert_row);
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

std::pair<model::VisualNode *, int> item_models::PropertyModelSingle::drop_position(const QModelIndex&, int, int) const
{
    return {};
}
