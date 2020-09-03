#include "layer.hpp"

#include <QPainter>

#include "model/composition.hpp"
#include "model/document.hpp"

void model::Layer::ChildLayerIterator::find_first()
{
    while ( index < comp->layers.size() && comp->layers[index].parent.get() != parent )
        ++index;
}

model::Layer & model::Layer::ChildLayerIterator::operator*() const
{
    return comp->layers[index];
}

model::Layer * model::Layer::ChildLayerIterator::operator->() const
{
    return &comp->layers[index];
}

model::Layer::Layer(Document* doc, Composition* composition)
    : AnimationContainer(doc), composition_(composition)
{
    connect(transform.get(), &Object::property_changed, this, &Layer::on_transform_matrix_changed);
}

utils::Range<model::Layer::ChildLayerIterator> model::Layer::children() const
{
    return {
        ChildLayerIterator(composition_, this, 0),
        ChildLayerIterator(composition_, this, composition_->docnode_child_count()),
    };
}


model::DocumentNode * model::Layer::docnode_parent() const
{
    return composition_;
}

std::vector<model::DocumentNode *> model::Layer::valid_parents() const
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

bool model::Layer::is_valid_parent(model::DocumentNode* node) const
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

void model::Layer::on_property_changed ( const BaseProperty* prop, const QVariant& value )
{
    if ( prop == &parent )
        docnode_on_update_group();
    else
        DocumentNode::on_property_changed(prop, value);
}

QTransform model::Layer::transform_matrix() const
{
    if ( parent.get() )
        return transform.get()->transform_matrix() * parent.get()->transform_matrix();
    return transform.get()->transform_matrix();
}

QTransform model::Layer::transform_matrix(model::FrameTime t) const
{
    if ( parent.get() )
        return transform.get()->transform_matrix(t) * parent.get()->transform_matrix(t);
    return transform.get()->transform_matrix(t);
}

QRectF model::Layer::local_bounding_rect(FrameTime) const
{
    return QRectF(QPointF(0, 0), QSizeF(document()->size()));
}

// QPolygonF model::Layer::unaligned_bounding_rect(FrameTime t) const
// {
//     QTransform tf = transform.get()->transform_matrix(t);
//     QRectF rect = local_bounding_rect(t);
//     return QPolygonF({
//         tf.map(rect.topLeft()),
//         tf.map(rect.topRight()),
//         tf.map(rect.bottomRight()),
//         tf.map(rect.bottomLeft())
//     });
// }
//
// QRectF model::Layer::bounding_rect(FrameTime t) const
// {
//     QTransform tf = transform.get()->transform_matrix(t);
//     QRectF rect = local_bounding_rect(t);
//     return tf.mapRect(rect);
// }

void model::Layer::on_paint(QPainter* painter, FrameTime time, PaintMode mode) const
{
    if ( mode != NoTransform )
        painter->setTransform(transform_matrix(time), true);
    on_paint_untransformed(painter, time);
}

void model::Layer::on_transform_matrix_changed()
{
    emit transform_matrix_changed(transform_matrix());
}

void model::Layer::set_time(model::FrameTime t)
{
    Object::set_time(relative_time(t));
}
