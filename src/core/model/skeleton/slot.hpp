#pragma once

#include "model/property/object_list_property.hpp"
#include "model/animation/animatable_reference.hpp"
#include "bone.hpp"

namespace glaxnimate::model {

class SkinSlot;

class SkinAttachment : public VisualNode
{
    GLAXNIMATE_OBJECT(SkinAttachment)
    Q_PROPERTY(glaxnimate::model::SkinSlot* slot READ slot)

public:
    using VisualNode::VisualNode;

    int docnode_child_count() const override { return 0; }
    DocumentNode* docnode_child(int) const override { return nullptr; }
    int docnode_child_index(DocumentNode*) const override { return -1; }
    QRectF local_bounding_rect(FrameTime) const override { return {}; }

    QIcon tree_icon() const override;
    QIcon instance_icon() const override;
    QString type_name_human() const override;

    int docnode_group_child_count() const override;
    VisualNode* docnode_group_child(int index) const override;


    void prepare_painter(QPainter* painter, model::FrameTime t) const;
    SkinSlot* slot() const;

protected:
    void on_parent_changed(model::DocumentNode* old_parent, model::DocumentNode* new_parent) override;
    void on_skeleton_changed(model::Skeleton* old_skel, model::Skeleton* new_skel);

    friend SkinSlot;
};

class SkinSlot : public BoneItem
{
    GLAXNIMATE_OBJECT(SkinSlot)
    GLAXNIMATE_SUBOBJECT(StaticTransform, initial)
    GLAXNIMATE_ANIMATABLE(int, draw_order, 0)
    GLAXNIMATE_ANIMATABLE(float, opacity, 1, {}, 0, 1, false, PropertyTraits::Percent)
    GLAXNIMATE_PROPERTY_LIST(SkinAttachment, attachments)
    GLAXNIMATE_ANIMATABLE_REFERENCE(SkinAttachment, attachment, &SkinSlot::valid_attachments, &SkinSlot::is_valid_attachment)

public:
    using BoneItem::BoneItem;

    QIcon tree_icon() const override;
    QString type_name_human() const override { return tr("Slot"); }

    int docnode_child_count() const override { return 0; }
    DocumentNode* docnode_child(int) const override { return nullptr; }
    int docnode_child_index(DocumentNode*) const override { return -1; }
    QRectF local_bounding_rect(FrameTime) const override { return {}; }

protected:
    std::vector<DocumentNode*> valid_attachments() const;
    bool is_valid_attachment(DocumentNode* node) const;

    void on_skeleton_changed(model::Skeleton* old_skel, model::Skeleton* new_skel) override;
};

} // namespace glaxnimate::model

