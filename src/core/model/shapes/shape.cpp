/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "shape.hpp"
#include "utils/range.hpp"
#include "styler.hpp"
#include "path.hpp"
#include "model/animation/join_animatables.hpp"

using namespace glaxnimate;

class glaxnimate::model::ShapeElement::Private
{
public:
    ShapeListProperty* property = nullptr;
    int position = -1;
    glaxnimate::model::Composition* owner_composition = nullptr;
    PathCache<QPainterPath> cached_path;

    void update_comp(glaxnimate::model::Composition* comp, ShapeElement* parent)
    {
        if ( comp != owner_composition )
        {
            auto old = owner_composition;
            owner_composition = comp;
            parent->on_composition_changed(old, comp);
        }
    }
};

glaxnimate::model::ShapeElement::ShapeElement(glaxnimate::model::Document* document)
    : VisualNode(document), d(std::make_unique<Private>())
{
}

glaxnimate::model::ShapeElement::~ShapeElement() = default;


glaxnimate::model::ShapeListProperty * glaxnimate::model::ShapeElement::owner() const
{
    return d->property;
}

void glaxnimate::model::ShapeElement::clear_owner()
{
    d->property = nullptr;
    d->position = -1;
    d->owner_composition = nullptr;
}

glaxnimate::model::Composition * glaxnimate::model::ShapeElement::owner_composition() const
{
    return d->owner_composition;
}

int glaxnimate::model::ShapeElement::position() const
{
    return d->position;
}

const glaxnimate::model::ShapeListProperty& glaxnimate::model::ShapeElement::siblings() const
{
    return *d->property;
}

glaxnimate::model::ObjectListProperty<glaxnimate::model::ShapeElement>::iterator glaxnimate::model::ShapeListProperty::past_first_modifier() const
{
    auto it = std::find_if(begin(), end(), [](const pointer& p){
        return qobject_cast<Modifier*>(p.get());
    });
    if ( it != end() )
        ++it;
    return it;
}

void glaxnimate::model::ShapeElement::set_position(ShapeListProperty* property, int pos)
{
    d->property = property;
    d->position = pos;
    position_updated();

    if ( property )
    {
        auto parent = d->property->object();
        if ( !parent )
            d->update_comp(nullptr, this);
        else if ( auto comp = parent->cast<glaxnimate::model::Composition>() )
            d->update_comp(comp, this);
        else if ( auto sh = parent->cast<glaxnimate::model::ShapeElement>() )
            d->update_comp(sh->d->owner_composition, this);
    }
}

void glaxnimate::model::ShapeElement::on_parent_changed(model::DocumentNode* old_parent, model::DocumentNode* new_parent)
{
    if ( auto old_visual = qobject_cast<model::VisualNode*>(old_parent) )
        disconnect(this, &VisualNode::bounding_rect_changed, old_visual, &VisualNode::bounding_rect_changed);

    if ( auto new_visual = qobject_cast<model::VisualNode*>(new_parent) )
        connect(this, &VisualNode::bounding_rect_changed, new_visual, &VisualNode::bounding_rect_changed);

    if ( !new_parent )
        d->update_comp(nullptr, this);
}

void glaxnimate::model::ShapeElement::on_property_changed(const glaxnimate::model::BaseProperty* prop, const QVariant&)
{
    if ( prop->traits().flags & PropertyTraits::Visual )
        propagate_bounding_rect_changed();
}

math::bezier::MultiBezier glaxnimate::model::ShapeElement::shapes(glaxnimate::model::FrameTime t) const
{
    math::bezier::MultiBezier bez;
    add_shapes(t, bez, {});
    return bez;
}

QPainterPath glaxnimate::model::ShapeElement::to_clip(FrameTime t) const
{
    return to_painter_path(t);
}

QPainterPath glaxnimate::model::ShapeElement::to_painter_path(FrameTime t) const
{
    if ( d->cached_path.is_dirty(t) )
        d->cached_path.set_path(t, to_painter_path_impl(t));
    return d->cached_path.path();
}

void glaxnimate::model::ShapeElement::on_graphics_changed()
{
    d->cached_path.mark_dirty();
}

std::unique_ptr<glaxnimate::model::ShapeElement> glaxnimate::model::ShapeElement::to_path() const
{
    return std::unique_ptr<glaxnimate::model::ShapeElement>(static_cast<glaxnimate::model::ShapeElement*>(clone().release()));
}

QRectF glaxnimate::model::ShapeListProperty::bounding_rect(FrameTime t) const
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

std::unique_ptr<glaxnimate::model::ShapeElement> glaxnimate::model::Shape::to_path() const
{
    std::vector<const AnimatableBase*> properties;
    auto flags = PropertyTraits::Visual|PropertyTraits::Animated;
    for ( auto prop : this->properties() )
    {
        if ( (prop->traits().flags & flags) == flags )
            properties.push_back(static_cast<AnimatableBase*>(prop));
    }

    auto path = std::make_unique<glaxnimate::model::Path>(document());
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
        path->closed.set(path->shape.get().closed());
    }

    return path;
}

QPainterPath glaxnimate::model::Shape::to_painter_path_impl(FrameTime t) const
{
    QPainterPath p;
    to_bezier(t).add_to_painter_path(p);
    return p;
}


void glaxnimate::model::Shape::add_shapes(FrameTime t, math::bezier::MultiBezier & bez, const QTransform& transform) const
{
    auto shape = to_bezier(t);
    if ( !transform.isIdentity() )
        shape.transform(transform);
    bez.beziers().emplace_back(std::move(shape));
}


glaxnimate::model::ShapeOperator::ShapeOperator(glaxnimate::model::Document* doc)
    : ShapeElement(doc)
{
    connect(this, &ShapeElement::position_updated, this, &ShapeOperator::update_affected);
    connect(this, &ShapeElement::siblings_changed, this, &ShapeOperator::update_affected);
}


void glaxnimate::model::ShapeOperator::do_collect_shapes(const std::vector<ShapeElement*>& shapes, glaxnimate::model::FrameTime t, math::bezier::MultiBezier& bez, const QTransform& transform) const
{
    for ( auto sib : shapes )
    {
        if ( sib->visible.get() )
            sib->add_shapes(t, bez, transform);
    }
}

math::bezier::MultiBezier glaxnimate::model::ShapeOperator::collect_shapes_from(const std::vector<ShapeElement *>& shapes, glaxnimate::model::FrameTime t, const QTransform& transform) const
{
    math::bezier::MultiBezier bez;
    if ( visible.get() )
        do_collect_shapes(shapes, t, bez, transform);
    return bez;
}


math::bezier::MultiBezier glaxnimate::model::ShapeOperator::collect_shapes(FrameTime t, const QTransform& transform) const
{
    if ( bezier_cache.is_dirty(t) )
        bezier_cache.set_path(t, collect_shapes_from(affected_elements, t, transform));
    return bezier_cache.path();
}

void glaxnimate::model::ShapeOperator::update_affected()
{
    if ( !owner() )
        return;

    std::vector<ShapeElement*> curr_siblings;
    curr_siblings.reserve(owner()->size() - position());
    bool skip = skip_stylers();
    for ( auto it = owner()->begin() + position() + 1; it < owner()->end(); ++it )
    {
        if ( skip && qobject_cast<Styler*>(it->get()) )
            continue;

        curr_siblings.push_back(it->get());
        if ( qobject_cast<Modifier*>(it->get()) )
            break;
    }

    affected_elements = curr_siblings;
}

void glaxnimate::model::ShapeOperator::on_graphics_changed()
{
    ShapeElement::on_graphics_changed();
    bezier_cache.mark_dirty();
    emit shape_changed();
}

void glaxnimate::model::Modifier::add_shapes(FrameTime t, math::bezier::MultiBezier& bez, const QTransform& transform) const
{
    bez.append(collect_shapes(t, transform));
}

void glaxnimate::model::Modifier::do_collect_shapes(const std::vector<ShapeElement*>& shapes, glaxnimate::model::FrameTime t, math::bezier::MultiBezier& bez, const QTransform& transform) const
{
    bool post = process_collected();

    if ( post )
    {
        math::bezier::MultiBezier temp;
        for ( auto sib : shapes )
        {
            if ( sib->visible.get() )
                sib->add_shapes(t, temp, transform);
        }

        bez.append(process(t, temp));
    }
    else
    {
        for ( auto sib : shapes )
        {
            if ( sib->visible.get() )
            {
                math::bezier::MultiBezier temp;
                sib->add_shapes(t, temp, transform);
                bez.append(process(t, temp));
            }
        }
    }
}

QPainterPath glaxnimate::model::Modifier::to_painter_path_impl(glaxnimate::model::FrameTime t) const
{
    math::bezier::MultiBezier bez;
    add_shapes(t, bez, {});
    return bez.painter_path();
}

QRectF glaxnimate::model::Modifier::local_bounding_rect(glaxnimate::model::FrameTime t) const
{
    return to_painter_path(t).boundingRect();
}


