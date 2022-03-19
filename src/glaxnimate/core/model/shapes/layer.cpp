#include "layer.hpp"

#include <QPainter>

#include "glaxnimate/core/model/composition.hpp"
#include "glaxnimate/core/model/document.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Layer)

void glaxnimate::model::Layer::ChildLayerIterator::find_first()
{
    while ( index < comp->size() && (*comp)[index]->docnode_group_parent() != parent )
        ++index;
}

glaxnimate::model::VisualNode* glaxnimate::model::Layer::ChildLayerIterator::operator*() const
{
    return (*comp)[index];
}

glaxnimate::model::VisualNode* glaxnimate::model::Layer::ChildLayerIterator::operator->() const
{
    return (*comp)[index];
}

glaxnimate::model::VisualNode * glaxnimate::model::Layer::docnode_group_parent() const
{
    return parent.get();
}

glaxnimate::model::VisualNode * glaxnimate::model::Layer::docnode_group_child(int index) const
{
    ChildLayerIterator iter(owner(), this, 0);
    std::advance(iter, index);
    return *iter;
}

int glaxnimate::model::Layer::docnode_group_child_count() const
{
    if ( !owner() )
        return 0;
    int sz = 0;
    for ( const auto& sib : *owner() )
        if ( sib->docnode_group_parent() == this )
            sz++;
    return sz;
}

std::vector<glaxnimate::model::DocumentNode*> glaxnimate::model::Layer::valid_parents() const
{
    std::vector<glaxnimate::model::DocumentNode*> refs;
    refs.push_back(nullptr);

    if ( is_top_level() )
    {
        for ( const auto& sh : *owner() )
        {
            if ( auto lay = qobject_cast<glaxnimate::model::Layer*>(sh.get()) )
                if ( !is_ancestor_of(lay) )
                    refs.push_back(lay);
        }
    }

    return refs;
}

bool glaxnimate::model::Layer::is_valid_parent(glaxnimate::model::DocumentNode* node) const
{
    if ( node == nullptr )
        return true;

    if ( is_top_level() )
    {
        if ( Layer* layer = qobject_cast<Layer*>(node) )
            return !is_ancestor_of(layer);
    }

    return false;
}

bool glaxnimate::model::Layer::is_ancestor_of ( const glaxnimate::model::Layer* other ) const
{
    while ( other )
    {
        if ( other == this )
            return true;

        other = other->parent.get();
    }

    return false;
}

void glaxnimate::model::Layer::set_time(glaxnimate::model::FrameTime t)
{
    Object::set_time(relative_time(t));
}


bool glaxnimate::model::Layer::is_top_level() const
{
    return qobject_cast<Composition*>(docnode_parent());
}

void glaxnimate::model::Layer::paint(QPainter* painter, FrameTime time, PaintMode mode, glaxnimate::model::Modifier* modifier) const
{
    if ( !visible.get() || (mode == Render && !render.get()) )
        return;

    time = relative_time(time);
    if ( !animation->time_visible(time) )
        return;

    if ( mask->has_mask() )
    {
        auto n_shapes = shapes.size();
        if ( n_shapes <= 1 )
            return;

        painter->save();
        auto transform = group_transform_matrix(time);
        painter->setTransform(transform, true);

        if ( shapes[0]->visible.get() )
        {
            QPainterPath clip = shapes[0]->to_clip(time);
            clip.setFillRule(Qt::WindingFill);
            if ( mask->inverted.get() )
            {
                QPainterPath outer_clip;
                outer_clip.addPolygon(
                    transform.inverted().map(QRectF(QPointF(0, 0), document()->size()))
                );
                clip = outer_clip.subtracted(clip);
            }
            painter->setClipPath(clip, Qt::IntersectClip);
        }


        on_paint(painter, time, mode, modifier);

        for ( int i = 1; i < n_shapes; i++ )
            docnode_visual_child(i)->paint(painter, time, mode);

        painter->restore();
    }
    else
    {
        VisualNode::paint(painter, time, mode);
    }
}

QPainterPath glaxnimate::model::Layer::to_clip(glaxnimate::model::FrameTime time) const
{
    time = relative_time(time);
    if ( !animation->time_visible(time) || !render.get() )
        return {};

    return Group::to_clip(time);
}

QPainterPath glaxnimate::model::Layer::to_painter_path(glaxnimate::model::FrameTime time) const
{
    time = relative_time(time);
    if ( !animation->time_visible(time) || !render.get() )
        return {};

    return Group::to_painter_path(time);
}



QIcon glaxnimate::model::Layer::tree_icon() const
{
    return mask->has_mask() ? QIcon::fromTheme("path-clip-edit") : QIcon::fromTheme("folder");
}

QIcon glaxnimate::model::Layer::static_tree_icon()
{
    return QIcon::fromTheme("folder");
}

std::unique_ptr<glaxnimate::model::ShapeElement> glaxnimate::model::Layer::to_path() const
{
    auto clone = std::make_unique<glaxnimate::model::Layer>(document());

    for ( BaseProperty* prop : properties() )
    {
        if ( prop != &shapes )
            clone->get_property(prop->name())->assign_from(prop);
    }

    for ( const auto& shape : shapes )
    {
        clone->shapes.insert(shape->to_path());
        if ( shape->is_instance<glaxnimate::model::Modifier>() )
            break;
    }

    return clone;
}
