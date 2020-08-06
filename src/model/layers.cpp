#include "layers.hpp"

#include <QPainter>

#include "composition.hpp"
#include "model/document.hpp"
#include "model/graphics/transform_graphics_item.hpp"

model::Layer::Layer(Document* doc, Composition* composition)
    : AnimationContainer(doc), composition_(composition)
{
    connect(transform.get(), &Object::property_changed, this, &Layer::on_transform_matrix_changed);
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

void model::Layer::on_paint(QPainter* painter, FrameTime time) const
{
    painter->setTransform(transform_matrix(time), true);
    on_paint_untransformed(painter, time);
}

std::vector<std::unique_ptr<QGraphicsItem>> model::Layer::docnode_make_graphics_editor()
{
    std::vector<std::unique_ptr<QGraphicsItem>> v;
    auto p = std::make_unique<model::graphics::TransformGraphicsItem>(transform.get(), this, nullptr);
    connect(this, &Layer::transform_matrix_changed, p.get(), &graphics::TransformGraphicsItem::set_transform_matrix);
    p->set_transform_matrix(transform_matrix());
    v.push_back(std::move(p));
    return v;
}

void model::Layer::on_transform_matrix_changed()
{
    emit transform_matrix_changed(transform_matrix());
}

model::SolidColorLayer::SolidColorLayer ( model::Document* doc, model::Composition* composition )
    : Ctor(doc, composition)
{
    width.set(doc->animation()->width.get());
    height.set(doc->animation()->height.get());
}


void model::SolidColorLayer::on_paint_untransformed(QPainter* painter, FrameTime time) const
{
    painter->fillRect(local_bounding_rect(time), color.get());
}

QRectF model::SolidColorLayer::local_bounding_rect(FrameTime) const
{
    return QRectF(0, 0, width.get(), height.get());
}



