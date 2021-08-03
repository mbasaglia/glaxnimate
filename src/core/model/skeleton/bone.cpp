#include "bone.hpp"

#include <QPainter>

#include "skeleton.hpp"
#include "command/animation_commands.hpp"
#include "command/object_list_commands.hpp"

// GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::SkinItem)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Bone)


glaxnimate::model::Skeleton * glaxnimate::model::BoneItem::skeleton() const
{
    return skeleton_;
}


void glaxnimate::model::BoneItem::on_parent_changed ( model::DocumentNode*, model::DocumentNode* new_parent )
{
    if ( !new_parent )
        skeleton_ = nullptr;
    else if ( auto skel = new_parent->cast<Skeleton>() )
        skeleton_ = skel;
    else
        skeleton_ = static_cast<BoneItem*>(new_parent)->skeleton_;
}

glaxnimate::model::Bone * glaxnimate::model::BoneItem::parent_bone() const
{
    return qobject_cast<Bone*>(docnode_parent());
}



QPointF glaxnimate::model::Bone::tip() const
{
    return position.get() + math::from_polar<QPointF>(length.get(), math::deg2rad(angle.get()));
}

void glaxnimate::model::Bone::set_tip ( const QPointF& p, bool commit )
{
    math::PolarVector v(p - position.get());
    command::SetMultipleAnimated* sma = new command::SetMultipleAnimated(tr("Set bone tip"), document(), commit);
    sma->push_property_not_animated(&length, v.length);
    sma->push_property_not_animated(&angle, math::deg2rad(v.angle));
    push_command(sma);
}

QTransform glaxnimate::model::Bone::local_transform_matrix ( glaxnimate::model::FrameTime t ) const
{
    return bone_transform(t);
}

QTransform glaxnimate::model::Bone::bone_transform ( glaxnimate::model::FrameTime t ) const
{
    return transform->transform_matrix(t);
}

void glaxnimate::model::Bone::on_paint(QPainter* painter, glaxnimate::model::FrameTime t, glaxnimate::model::VisualNode::PaintMode mode, model::Modifier* ) const
{
    if  ( mode == Render )
        return;

    QPen pen(color.get(), 2);
    pen.setCosmetic(true);
    painter->setPen(pen);

    QPointF pos = position.get();
    QPointF start = bone_transform(t).map(pos);
    pos.setX(pos.x() + 8);
    auto radius = math::length(start - bone_transform(t).map(pos));

    if ( length.get() == 0 )
    {
        painter->drawEllipse(start, radius, radius);
    }
    else
    {
        QPointF finish = bone_transform(t).map(tip());
        auto angle = math::angle(finish - start);
        auto angle_delta = math::pi / 6;
        QPolygonF polygon{QVector<QPointF>{
            start,
            start + math::from_polar<QPointF>(radius, angle+angle_delta),
            finish,
            start + math::from_polar<QPointF>(radius, angle-angle_delta),
        }};
        painter->drawPolygon(polygon);
    }
}

glaxnimate::model::VisualNode * glaxnimate::model::Bone::docnode_child ( int index ) const
{
    return children[index];
}

int glaxnimate::model::Bone::docnode_child_count() const
{
    return children.size();
}

int glaxnimate::model::Bone::docnode_child_index(glaxnimate::model::DocumentNode* child) const
{
    return children.index_of(static_cast<model::Bone*>(child));
}


QRectF glaxnimate::model::Bone::local_bounding_rect(FrameTime t) const
{
    return range_bounding_rect(t, children.begin(), children.end(), QRectF(position.get(), tip()).normalized());
}

QIcon glaxnimate::model::Bone::tree_icon() const
{
    return QIcon::fromTheme("bone");
}

glaxnimate::model::Bone * glaxnimate::model::Bone::add_bone()
{
    auto child = std::make_unique<Bone>(document());
    auto raw = child.get();
    push_command(new command::AddObject<BoneItem>(&children, std::move(child), children.size()));
    return raw;
}
