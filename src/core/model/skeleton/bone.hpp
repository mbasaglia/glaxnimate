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


class StaticTransform : public Object
{
    GLAXNIMATE_OBJECT(StaticTransform)
    GLAXNIMATE_PROPERTY(QPointF, position, QPointF(0, 0), {}, {}, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(QVector2D, scale, QVector2D(1, 1), {}, {}, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(float, rotation, 0, {}, {}, PropertyTraits::Visual)

public:
    using Object::Object;

    virtual QString type_name_human() const override { return tr("Static Transform"); }

    QTransform transform_matrix() const;
};

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
    Skeleton* skeleton_from_parent(model::DocumentNode* parent) const;
    virtual void on_skeleton_changed(model::Skeleton* old_skel, model::Skeleton* new_skel) = 0;

private:
    Skeleton* skeleton_ = nullptr;
};

class SkinSlot : public BoneItem
{
    GLAXNIMATE_OBJECT(SkinSlot)
    GLAXNIMATE_SUBOBJECT(StaticTransform, initial)

public:
    using BoneItem::BoneItem;

    QIcon tree_icon() const override;
    QString type_name_human() const override { return tr("Slot"); }

    int docnode_child_count() const override { return 0; }
    DocumentNode* docnode_child(int) const override { return nullptr; }
    int docnode_child_index(DocumentNode*) const override { return -1; }
    QRectF local_bounding_rect(FrameTime) const override { return {}; }

protected:
    void on_skeleton_changed(model::Skeleton* old_skel, model::Skeleton* new_skel) override;
};

class BoneDisplay : public Object
{
    GLAXNIMATE_OBJECT(BoneDisplay)
    GLAXNIMATE_PROPERTY(float, length, 0, {}, &BoneDisplay::valid_length, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(QColor, color, QColor(200, 120, 0), {}, {}, PropertyTraits::Visual)

public:
    using Object::Object;

    virtual QString type_name_human() const override { return tr("Bone Display"); }

private:
    bool valid_length(float length) const
    {
        return length >= 0;
    }
};

class Bone : public BoneItem
{
    GLAXNIMATE_OBJECT(Bone)
    GLAXNIMATE_SUBOBJECT(BoneDisplay, display)
    GLAXNIMATE_SUBOBJECT(StaticTransform, initial)
    GLAXNIMATE_SUBOBJECT(Transform, pose)
    GLAXNIMATE_PROPERTY_LIST(BoneItem, children)
    Q_PROPERTY(QPointF tip WRITE set_tip READ tip)

public:
    explicit Bone(Document* document);

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
    void on_skeleton_changed(model::Skeleton* old_skel, model::Skeleton* new_skel) override;

private:
    void on_transform_matrix_changed();
};

} // namespace glaxnimate::model
