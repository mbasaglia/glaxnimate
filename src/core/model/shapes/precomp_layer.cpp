#include "precomp_layer.hpp"

#include <QPainter>

#include "model/document.hpp"
#include "model/assets/precomposition.hpp"
#include "model/assets/assets.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::PreCompLayer)

glaxnimate::model::PreCompLayer::PreCompLayer(Document* document)
    : ShapeElement(document)
{
    connect(transform.get(), &Object::property_changed, this, &PreCompLayer::on_transform_matrix_changed);
}

QIcon glaxnimate::model::PreCompLayer::tree_icon() const
{
    return QIcon::fromTheme("component");
}

QString glaxnimate::model::PreCompLayer::type_name_human() const
{
    return tr("Composition Layer");
}

glaxnimate::model::FrameTime glaxnimate::model::PreCompLayer::relative_time(glaxnimate::model::FrameTime time) const
{
    return timing->time_to_local(time);
}

void glaxnimate::model::PreCompLayer::set_time(glaxnimate::model::FrameTime t)
{
    ShapeElement::set_time(relative_time(t));
    emit document()->graphics_invalidated();
}

std::vector<glaxnimate::model::DocumentNode *> glaxnimate::model::PreCompLayer::valid_precomps() const
{
    auto comps = document()->comp_graph().possible_descendants(owner_composition(), document());
    return std::vector<glaxnimate::model::DocumentNode *>(comps.begin(), comps.end());
}

bool glaxnimate::model::PreCompLayer::is_valid_precomp(glaxnimate::model::DocumentNode* node) const
{
    auto owncomp = owner_composition();
    if ( auto precomp = qobject_cast<glaxnimate::model::Precomposition*>(node) )
        return !document()->comp_graph().is_ancestor_of(precomp, owncomp);
    return false;
}

void glaxnimate::model::PreCompLayer::on_paint(QPainter* painter, glaxnimate::model::FrameTime time, glaxnimate::model::VisualNode::PaintMode mode, glaxnimate::model::Modifier*) const
{
    if ( composition.get() )
    {
        time = timing->time_to_local(time);
        painter->setOpacity(
            painter->opacity() * opacity.get_at(time)
        );
        painter->setClipRect(QRectF(QPointF(0, 0), size.get()), Qt::IntersectClip);
        composition->paint(painter, time, mode);
    }
}

void glaxnimate::model::PreCompLayer::on_transform_matrix_changed()
{
    propagate_bounding_rect_changed();
    emit local_transform_matrix_changed(local_transform_matrix(time()));
    propagate_transform_matrix_changed(transform_matrix(time()), group_transform_matrix(time()));
}

QRectF glaxnimate::model::PreCompLayer::local_bounding_rect(FrameTime) const
{
    return QRectF(QPointF(0, 0), size.get());
}

QTransform glaxnimate::model::PreCompLayer::local_transform_matrix(glaxnimate::model::FrameTime t) const
{
    return transform.get()->transform_matrix(t);
}

void glaxnimate::model::PreCompLayer::add_shapes(glaxnimate::model::FrameTime, math::bezier::MultiBezier&, const QTransform&) const
{
}

void glaxnimate::model::PreCompLayer::on_composition_changed(model::Composition* old_comp, model::Composition* new_comp)
{
    if ( old_comp )
        document()->comp_graph().remove_connection(old_comp, this);

    if ( new_comp )
        document()->comp_graph().add_connection(new_comp, this);

    if ( composition.get() )
    {
        if ( !new_comp )
            composition->remove_user(&composition);
        else if ( !old_comp )
            composition->add_user(&composition);
    }
}

QPainterPath glaxnimate::model::PreCompLayer::to_painter_path_impl(glaxnimate::model::FrameTime time) const
{
    QPainterPath p;
    if ( composition.get() )
    {
        time = timing->time_to_local(time);
        for ( const auto& sh : composition->shapes )
            p.addPath(sh->to_clip(time));
    }
    return p;
}

QPainterPath glaxnimate::model::PreCompLayer::to_clip(glaxnimate::model::FrameTime time) const
{
    return transform.get()->transform_matrix(time).map(to_painter_path(time));
}
