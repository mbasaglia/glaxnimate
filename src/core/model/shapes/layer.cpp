#include "layer.hpp"

#include <QPainter>

#include "model/composition.hpp"
#include "model/document.hpp"
#include "model/defs/defs.hpp"
#include "model/defs/precomposition.hpp"

GLAXNIMATE_OBJECT_IMPL(model::Layer)

void model::Layer::ChildLayerIterator::find_first()
{
    while ( index < comp->size() && (*comp)[index]->docnode_group_parent() != parent )
        ++index;
}

model::DocumentNode * model::Layer::ChildLayerIterator::operator*() const
{
    return (*comp)[index];
}

model::DocumentNode* model::Layer::ChildLayerIterator::operator->() const
{
    return (*comp)[index];
}

model::DocumentNode * model::Layer::docnode_group_parent() const
{
    return parent.get();
}

model::DocumentNode * model::Layer::docnode_group_child(int index) const
{
    ChildLayerIterator iter(owner(), this, 0);
    std::advance(iter, index);
    return *iter;
}

int model::Layer::docnode_group_child_count() const
{
    if ( !owner() )
        return 0;
    int sz = 0;
    for ( const auto& sib : *owner() )
        if ( sib->docnode_group_parent() == this )
            sz++;
    return sz;
}

std::vector<model::ReferenceTarget*> model::Layer::valid_parents() const
{
    std::vector<model::ReferenceTarget*> refs;
    refs.push_back(nullptr);

    if ( is_top_level() )
    {
        for ( const auto& sh : *owner() )
        {
            if ( auto lay = qobject_cast<model::Layer*>(sh.get()) )
                if ( !is_ancestor_of(lay) )
                    refs.push_back(lay);
        }
    }

    return refs;
}

bool model::Layer::is_valid_parent(model::ReferenceTarget* node) const
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

void model::Layer::set_time(model::FrameTime t)
{
    Object::set_time(relative_time(t));
}


bool model::Layer::is_top_level() const
{
    return qobject_cast<Composition*>(docnode_parent());
}

void model::Layer::paint(QPainter* painter, FrameTime time, PaintMode mode) const
{
    time = relative_time(time);
    if ( !animation->time_visible(time) )
        return;


    if ( mode != Render || render.get() )
    {
        if ( matte.get() )
        {
            painter->save();
            painter->setClipPath(matte->to_clip(time), Qt::IntersectClip);
            DocumentNode::paint(painter, time, mode);
            painter->restore();
        }
        else
        {
            DocumentNode::paint(painter, time, mode);
        }
    }
}

std::vector<model::ReferenceTarget*> model::Layer::valid_mattes() const
{
    return document()->defs()->mattes->shapes.valid_reference_values(true);
}

bool model::Layer::is_valid_matte(model::ReferenceTarget* node) const
{
    return document()->defs()->mattes->shapes.is_valid_reference_value(node, true);
}

QPainterPath model::Layer::to_local_clip(model::FrameTime time) const
{
    time = relative_time(time);
    if ( !animation->time_visible(time) )
        return {};

    return Group::to_local_clip(time);
}
