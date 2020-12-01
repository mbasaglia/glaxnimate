#include "precomp_layer.hpp"

#include <QPainter>

#include "model/document.hpp"
#include "model/defs/precomposition.hpp"
#include "model/defs/defs.hpp"

GLAXNIMATE_OBJECT_IMPL(model::PreCompLayer)

model::PreCompLayer::PreCompLayer(Document* document)
    : ShapeElement(document)
{
    connect(transform.get(), &Object::property_changed, this, &PreCompLayer::on_transform_matrix_changed);
}

QIcon model::PreCompLayer::docnode_icon() const
{
    return QIcon::fromTheme("component");
}

QString model::PreCompLayer::type_name_human() const
{
    return tr("Composition Layer");
}

model::FrameTime model::PreCompLayer::relative_time(model::FrameTime time) const
{
    return timing->time_to_local(time);
}

void model::PreCompLayer::set_time(model::FrameTime t)
{
    ShapeElement::set_time(relative_time(t));
}

std::vector<model::ReferenceTarget *> model::PreCompLayer::valid_precomps() const
{
    auto v = document()->defs()->precompositions.valid_reference_values(false);
    auto it = std::find(v.begin(), v.end(), owner_composition());
    if ( it != v.end() )
        v.erase(it);
    return v;
}

bool model::PreCompLayer::is_valid_precomp(model::ReferenceTarget* node) const
{
    return document()->defs()->precompositions.is_valid_reference_value(node, false) && owner_composition() != node;
}

void model::PreCompLayer::on_paint(QPainter* painter, model::FrameTime time, model::DocumentNode::PaintMode mode) const
{
    if ( composition.get() )
    {
        if ( mode != model::DocumentNode::NoTransform )
            time = timing->time_to_local(time);
        painter->setOpacity(
            painter->opacity() * opacity.get_at(time)
        );
        painter->setClipRect(QRectF(QPointF(0, 0), size.get()), Qt::IntersectClip);
        composition->paint(painter, time, qMax(mode, model::DocumentNode::Recursive));
    }
}

void model::PreCompLayer::on_transform_matrix_changed()
{
    emit bounding_rect_changed();
    emit local_transform_matrix_changed(local_transform_matrix(time()));
    propagate_transform_matrix_changed(transform_matrix(time()), group_transform_matrix(time()));
}

QRectF model::PreCompLayer::local_bounding_rect(FrameTime) const
{
    return QRectF(QPointF(0, 0), size.get());
}

QTransform model::PreCompLayer::local_transform_matrix(model::FrameTime t) const
{
    return transform.get()->transform_matrix(t);
}

void model::PreCompLayer::add_shapes(model::FrameTime, math::bezier::MultiBezier&) const
{
}

model::Composition* model::PreCompLayer::owner_composition() const
{
    auto n = docnode_parent();
    while ( n && !n->is_instance<model::Composition>() )
        n = n->docnode_parent();

    return static_cast<model::Composition*>(n);
}
