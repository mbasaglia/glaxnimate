#pragma once

#include "model/document_node.hpp"
#include "model/transform.hpp"
#include "model/property/sub_object_property.hpp"
#include "model/property/object_list_property.hpp"
#include "model/property/reference_property.hpp"
#include "model/shapes/shape.hpp"

namespace glaxnimate::model {

class Skeleton;
class Bone;

class BoneItem : public VisualNode
{
    Q_OBJECT
    Q_PROPERTY(glaxnimate::model::Skeleton* skeleton READ skeleton)

public:
    using VisualNode::VisualNode;

    Skeleton* skeleton() const;
    Bone* parent_bone() const;

protected:
    void on_parent_changed(model::DocumentNode* old_parent, model::DocumentNode* new_parent) override;

private:
    Skeleton* skeleton_ = nullptr;
};
/*
class SkinItem : public BoneItem
{
    GLAXNIMATE_OBJECT(SkinItem)
    GLAXNIMATE_PROPERTY_LIST(ShapeElement, shapes)
    GLAXNIMATE_SUBOBJECT(Transform, transform)

public:
    using BoneItem::BoneItem;
};
*/
class Bone : public BoneItem
{
    GLAXNIMATE_OBJECT(Bone)
    GLAXNIMATE_PROPERTY(QPointF, position, {0, 0}, {}, {}, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(float, length, 0, {}, &Bone::valid_length, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(float, angle, 0, {}, {}, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(QColor, color, QColor(200, 120, 0), {}, {}, PropertyTraits::Visual)
    GLAXNIMATE_SUBOBJECT(Transform, transform)
    GLAXNIMATE_PROPERTY_LIST(BoneItem, children)
    Q_PROPERTY(QPointF tip WRITE set_tip READ tip)

public:
    using BoneItem::BoneItem;

    QPointF tip() const;
    void set_tip(const QPointF& p, bool commit = true);
    QTransform local_transform_matrix(FrameTime t) const override;
    QTransform bone_transform(FrameTime t) const;

    int docnode_child_count() const override;
    VisualNode* docnode_child(int index) const override;
    int docnode_child_index(DocumentNode*) const override;
    QRectF local_bounding_rect(FrameTime t) const override;
    QIcon tree_icon() const override;
    QString type_name_human() const override { return tr("Bone"); }

    Q_INVOKABLE glaxnimate::model::Bone* add_bone();

protected:
    void on_paint(QPainter* painter, FrameTime t, PaintMode mode, model::Modifier*) const override;

private:
    bool valid_length(float length) const
    {
        return length >= 0;
    }
};

} // namespace glaxnimate::model
