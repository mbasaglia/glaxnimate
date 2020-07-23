#include "layers.hpp"

#include "composition.hpp"

model::Layer::Layer(Document* doc, Composition* composition)
    : DocumentNode(doc), composition_(composition)
{
}

void model::ChildLayerView::iterator::find_first()
{
    while ( index < comp->layers.size() && comp->layers[index].parent.get() != parent )
        ++index;
}

model::Layer & model::ChildLayerView::iterator::operator*() const
{
    return comp->layers[index];
}

model::Layer * model::ChildLayerView::iterator::operator->() const
{
    return &comp->layers[index];
}

model::DocumentNode * model::Layer::docnode_parent() const
{
    return composition_;
}

std::vector<model::DocumentNode *> model::Layer::docnode_valid_references ( const model::ReferencePropertyBase* ) const
{
    std::vector<model::DocumentNode *> refs;
    refs.push_back(nullptr);
    for ( const auto& lay : composition_->layers )
    {
        if ( !is_ancestor_of(lay.get()) )
            refs.push_back(lay.get());
    }

    return refs;
}

bool model::Layer::docnode_is_valid_reference ( const model::ReferencePropertyBase*, model::DocumentNode* node ) const
{
    if ( node == nullptr )
        return true;

    if ( Layer* layer = qobject_cast<Layer*>(node) )
        return !is_ancestor_of(layer);

    return false;
}

bool model::Layer::is_ancestor_of ( const model::Layer* other ) const
{
    while ( other )
    {
        if ( other == this )
            return true;

        other = other->parent.get();
    }

    return false;
}

model::DocumentNode * model::Layer::docnode_group_parent() const
{
    if ( parent.get() )
        return parent.get();

    return composition_;
}

void model::Layer::set_composition ( model::Composition* composition )
{
    if ( composition_ == composition || !composition )
        return;

    composition_ = composition;
    docnode_on_update_group();
}

void model::Layer::on_property_changed ( const QString& name, const QVariant& value )
{
    if ( name == "parent" )
        docnode_on_update_group();
    else
        DocumentNode::on_property_changed(name, value);
}
