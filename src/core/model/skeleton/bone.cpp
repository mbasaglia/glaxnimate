#include "bone.hpp"

#include <QPainter>

#include "skeleton.hpp"
#include "command/animation_commands.hpp"
#include "command/object_list_commands.hpp"

// GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::SkinItem)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Bone)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::BoneDisplay)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::StaticTransform)

QTransform glaxnimate::model::StaticTransform::transform_matrix() const
{
    QTransform trans;
    trans.translate(position.get().x(), position.get().y());
    trans.rotate(rotation.get());
    trans.scale(scale.get().x(), scale.get().y());
    return trans;
}


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

glaxnimate::model::Bone::Bone(glaxnimate::model::Document* document)
    : BoneItem(document)
{
    connect(transform.get(), &Object::visual_property_changed,
            this, &Bone::on_transform_matrix_changed);
    connect(initial.get(), &Object::visual_property_changed,
            this, &Bone::on_transform_matrix_changed);
}

void glaxnimate::model::Bone::on_transform_matrix_changed()
{
    emit bounding_rect_changed();
    emit local_transform_matrix_changed(bone_transform(time()));
    propagate_transform_matrix_changed(transform_matrix(time()), group_transform_matrix(time()));
}

QPointF glaxnimate::model::Bone::tip() const
{
    return initial->position.get() + math::from_polar<QPointF>(display->length.get(), math::deg2rad(initial->rotation.get()));
}

void glaxnimate::model::Bone::set_tip ( const QPointF& p, bool commit )
{
    math::PolarVector v(p - initial->position.get());
    command::SetMultipleAnimated* sma = new command::SetMultipleAnimated(tr("Set bone tip"), document(), commit);
    sma->push_property_not_animated(&display->length, v.length);
    sma->push_property_not_animated(&initial->rotation, math::deg2rad(v.angle));
    push_command(sma);
}

QTransform glaxnimate::model::Bone::local_transform_matrix ( glaxnimate::model::FrameTime t ) const
{
    return bone_transform(t);
}

QTransform glaxnimate::model::Bone::bone_transform ( glaxnimate::model::FrameTime t ) const
{
    return transform->transform_matrix(t) * initial->transform_matrix();
}

void glaxnimate::model::Bone::on_paint(QPainter* painter, glaxnimate::model::FrameTime t, glaxnimate::model::VisualNode::PaintMode mode, model::Modifier* ) const
{
    if  ( mode == Render )
        return;

    QPen pen(display->color.get(), 2);
    pen.setCosmetic(true);
    painter->setPen(pen);

    QPointF start;
    QPointF pos = start;
    QPointF start_scene = bone_transform(t).map(pos);
    pos.setX(pos.x() + 8);
    auto radius = math::length(start_scene - bone_transform(t).map(pos));

    if ( display->length.get() == 0 )
    {
        painter->drawEllipse(start, radius, radius);
    }
    else
    {
        QPointF finish = QPointF(display->length.get(), 0);
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
    return range_bounding_rect(t, children.begin(), children.end(), QRectF(QPointF(0, 0), QPointF(display->length.get(), 0)).normalized());
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
