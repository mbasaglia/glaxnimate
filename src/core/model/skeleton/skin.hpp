#pragma once

#include "slot.hpp"
#include "model/assets/bitmap.hpp"


namespace glaxnimate::model {


class Skin;

class SkinItem : public VisualNode
{
public:
    Q_OBJECT
    Q_PROPERTY(glaxnimate::model::Skin* skin READ skin)
    Q_PROPERTY(glaxnimate::model::Skeleton* skeleton READ skeleton)
    GLAXNIMATE_PROPERTY_REFERENCE(SkinAttachment, attachment, &SkinItem::valid_slots, &SkinItem::is_valid_slot, &SkinItem::on_slot_changed)

public:
    using VisualNode::VisualNode;

    Skin* skin() const;
    Skeleton* skeleton() const;

    int docnode_child_count() const override { return 0; }
    DocumentNode* docnode_child(int) const override { return nullptr; }
    int docnode_child_index(DocumentNode*) const override { return -1; }
    QString object_name() const override;

    SkinSlot* slot() const;


    virtual VisualNode* docnode_group_parent() const override;
    virtual int docnode_group_child_count() const override { return 0; }
    virtual VisualNode* docnode_group_child(int) const override { return nullptr; }

    model::Composition* owner_composition() const;

protected:
    void on_parent_changed(model::DocumentNode* old_parent, model::DocumentNode* new_parent) override;
    void on_slot_changed(glaxnimate::model::SkinAttachment* new_use, glaxnimate::model::SkinAttachment* old_use);

private:
    std::vector<DocumentNode*> valid_slots() const;
    bool is_valid_slot(DocumentNode* node) const;

    Skin* skin_ = nullptr;
};

class Skin : public VisualNode
{
    GLAXNIMATE_OBJECT(Skin)
    GLAXNIMATE_PROPERTY_LIST(SkinItem, items)
    Q_PROPERTY(glaxnimate::model::Skeleton* skeleton READ skeleton)

public:
    using VisualNode::VisualNode;

    Skeleton* skeleton() const;

    QIcon tree_icon() const override;
    QString type_name_human() const override;

    int docnode_child_count() const override;
    DocumentNode* docnode_child(int) const override;
    int docnode_child_index(DocumentNode*) const override;
    QRectF local_bounding_rect(FrameTime) const override { return {}; }

protected:
    void on_parent_changed(model::DocumentNode* old_parent, model::DocumentNode* new_parent) override;

private:
    Skeleton* skeleton_ = nullptr;
};

class ShapeSkin : public SkinItem
{
    GLAXNIMATE_OBJECT(ShapeSkin)
    GLAXNIMATE_SUBOBJECT(StaticTransform, transform)
    GLAXNIMATE_PROPERTY_LIST(ShapeElement, shapes)

public:
    ShapeSkin(Document* document);

    QIcon tree_icon() const override;
    QString type_name_human() const override;
    QTransform local_transform_matrix(model::FrameTime t) const override;

    QRectF local_bounding_rect(FrameTime t) const override
    {
        return shapes.bounding_rect(t);
    }

    DocumentNode* docnode_child(int index) const override
    {
        return shapes[index];
    }

    int docnode_child_count() const override
    {
        return shapes.size();
    }

    int docnode_child_index(DocumentNode* dn) const override
    {
        return shapes.index_of(dn);
    }

protected:
    void on_paint(QPainter* p, FrameTime t, PaintMode, model::Modifier*) const override;

private:
    void on_transform_matrix_changed();
};

} // namespace glaxnimate::model
