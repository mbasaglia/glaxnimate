#include "shape.hpp"
#include "utils/range.hpp"
#include "styler.hpp"
#include "path.hpp"
#include "model/animation/join_animatables.hpp"

const model::ShapeListProperty& model::ShapeElement::siblings() const
{
    return *property_;
}

model::DocumentNode * model::ShapeElement::docnode_parent() const
{
    return property_ ? static_cast<DocumentNode*>(property_->object()) : nullptr;
}

model::ObjectListProperty<model::ShapeElement>::iterator model::ShapeListProperty::past_first_modifier() const
{
    auto it = std::find_if(begin(), end(), [](const pointer& p){
        return qobject_cast<Modifier*>(p.get());
    });
    if ( it != end() )
        ++it;
    return it;
}

void model::ShapeElement::set_position(ShapeListProperty* property, int pos)
{
    model::DocumentNode* old_parent = docnode_parent();

    property_ = property;
    position_ = pos;
    position_updated();

    model::DocumentNode* new_parent = docnode_parent();
    if ( old_parent != new_parent )
    {
        if ( old_parent )
            disconnect(this, &DocumentNode::bounding_rect_changed, old_parent, &DocumentNode::bounding_rect_changed);

        if ( new_parent )
            connect(this, &DocumentNode::bounding_rect_changed, new_parent, &DocumentNode::bounding_rect_changed);
    }
}

void model::ShapeElement::on_property_changed(const model::BaseProperty* prop, const QVariant&)
{
    if ( prop->traits().flags & PropertyTraits::Visual )
        emit bounding_rect_changed();
}

math::bezier::MultiBezier model::ShapeElement::shapes(model::FrameTime t) const
{
    math::bezier::MultiBezier bez;
    add_shapes(t, bez);
    return bez;
}

QPainterPath model::ShapeElement::to_clip(FrameTime t) const
{
    return transform_matrix(t).map(to_local_clip(t));
}


QRectF model::ShapeListProperty::bounding_rect(FrameTime t) const
{
    QRectF rect;
    for ( const auto& ch : utils::Range(begin(), past_first_modifier()) )
    {
        QRectF local_rect = ch->local_bounding_rect(t);
        if ( local_rect.isNull() )
            continue;

        QRectF child_rect = ch->local_transform_matrix(t).map(local_rect).boundingRect();

        if ( rect.isNull() )
            rect = child_rect;
        else
            rect |= child_rect;
    }

    return rect;
}


std::unique_ptr<model::Path> model::Shape::to_path() const
{
    std::vector<AnimatableBase*> properties;
    auto flags = PropertyTraits::Visual|PropertyTraits::Animated;
    for ( auto prop : this->properties() )
    {
        if ( (prop->traits().flags & flags) == flags )
            properties.push_back(static_cast<AnimatableBase*>(prop));
    }

    auto path = std::make_unique<model::Path>(document());
    path->name.set(name.get());
    path->group_color.set(group_color.get());
    path->visible.set(visible.get());

    if ( !properties.empty() )
    {

        JoinAnimatables ja(std::move(properties));
        FrameTime cur_time = ja.properties()[0]->time();
        path->set_time(cur_time);

        if ( ja.animated() )
        {
            for ( const auto & kf : ja )
            {
                auto path_kf = path->shape.set_keyframe(kf.time, to_bezier(kf.time));
                path_kf->set_transition(kf.transition());
            }
        }

        path->shape.set(to_bezier(cur_time));
    }

    return path;
}

QPainterPath model::Shape::to_local_clip(FrameTime t) const
{
    QPainterPath p;
    to_bezier(t).add_to_painter_path(p);
    return p;
}


model::ShapeOperator::ShapeOperator(model::Document* doc)
    : ShapeElement(doc)
{
    connect(this, &ShapeElement::position_updated, this, &ShapeOperator::update_affected);
    connect(this, &ShapeElement::siblings_changed, this, &ShapeOperator::update_affected);
}


void model::ShapeOperator::collect_shapes(model::FrameTime t, math::bezier::MultiBezier& bez) const
{
    for ( auto sib : affected_elements )
    {
        sib->add_shapes(t, bez);
    }
}

void model::ShapeOperator::update_affected()
{
    if ( !owner() )
        return;

    std::vector<ShapeElement*> curr_siblings;
    curr_siblings.reserve(owner()->size() - position());
    for ( auto it = owner()->begin() + position() + 1; it < owner()->end(); ++it )
    {
        if ( qobject_cast<Styler*>(it->get()) )
            continue;

        curr_siblings.push_back(it->get());
        if ( qobject_cast<Modifier*>(it->get()) )
            break;
    }
    std::sort(curr_siblings.begin(), curr_siblings.end());

    std::vector<ShapeElement*> to_disconnect;
    std::set_difference(affected_elements.begin(), affected_elements.end(), curr_siblings.begin(), curr_siblings.end(), std::back_inserter(to_disconnect));
    for ( ShapeElement* sib : to_disconnect )
        disconnect(sib, &Object::property_changed, this, &ShapeOperator::sibling_prop_changed);

    std::vector<ShapeElement*> to_connect;
    std::set_difference(curr_siblings.begin(), curr_siblings.end(), affected_elements.begin(), affected_elements.end(), std::back_inserter(to_connect));
    for ( ShapeElement* sib : to_connect )
        connect(sib, &Object::property_changed, this, &ShapeOperator::sibling_prop_changed);

    affected_elements = curr_siblings;
}

void model::ShapeOperator::sibling_prop_changed(const model::BaseProperty* prop)
{
    if ( prop->traits().flags & model::PropertyTraits::Visual )
        emit shape_changed();
}
