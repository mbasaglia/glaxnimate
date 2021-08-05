#pragma once

#include "bone.hpp"
#include "model/assets/bitmap.hpp"


namespace glaxnimate::model {


class Skin;

class SkinItemBase : public VisualNode
{
public:
    enum Type
    {
        Image,
        Mesh,
        LinkedMesh,
        BoundingBox,
        Path,
        Point,
        Clipping
    };
    Q_OBJECT

    Q_ENUM(Type)

    Q_PROPERTY(Type type READ type)
    Q_PROPERTY(Skin* skin READ skin)
    Q_PROPERTY(Skeleton* skeleton READ skeleton)
    GLAXNIMATE_PROPERTY_REFERENCE(SkinSlot, slot, &SkinItemBase::valid_slots, &SkinItemBase::is_valid_slot, &SkinItemBase::on_slot_changed)

public:
    using VisualNode::VisualNode;

    virtual Type type() const = 0;
    Skin* skin() const;
    Skeleton* skeleton() const;

    int docnode_child_count() const override { return 0; }
    DocumentNode* docnode_child(int) const override { return nullptr; }
    int docnode_child_index(DocumentNode*) const override { return -1; }
    QString object_name() const override;


    virtual VisualNode* docnode_group_parent() const override;
    virtual int docnode_group_child_count() const override { return 0; }
    virtual VisualNode* docnode_group_child(int) const override { return nullptr; }

protected:
    void on_parent_changed(model::DocumentNode* old_parent, model::DocumentNode* new_parent) override;
    void on_slot_changed(glaxnimate::model::SkinSlot* new_use, glaxnimate::model::SkinSlot* old_use);

private:
    std::vector<DocumentNode*> valid_slots() const;
    bool is_valid_slot(DocumentNode* node) const;

    Skin* skin_ = nullptr;
};

class Skin : public VisualNode
{
    GLAXNIMATE_OBJECT(Skin)
    GLAXNIMATE_PROPERTY_LIST(SkinItemBase, items)
    Q_PROPERTY(Skeleton* skeleton READ skeleton)

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

template<SkinItemBase::Type item_type>
class SkinItem : public SkinItemBase
{
public:
    using SkinItemBase::SkinItemBase;

    Type type() const override
    {
        return item_type;
    }

protected:
    using Ctor = SkinItem;
};

class ImageSkin : public SkinItem<SkinItemBase::Image>
{
    GLAXNIMATE_OBJECT(ImageSkin)
    GLAXNIMATE_SUBOBJECT(StaticTransform, transform)
    GLAXNIMATE_PROPERTY_REFERENCE(Bitmap, image, &ImageSkin::valid_images, &ImageSkin::is_valid_image, &ImageSkin::on_image_changed)

public:
    ImageSkin(Document* document);

    QIcon tree_icon() const override;
    QString type_name_human() const override;
    QRectF local_bounding_rect(FrameTime t) const override;
    QTransform local_transform_matrix(model::FrameTime t) const override;

protected:
    void on_paint(QPainter* p, FrameTime t, PaintMode, model::Modifier*) const override;

private:
    std::vector<DocumentNode*> valid_images() const;
    bool is_valid_image(DocumentNode* node) const;
    void on_image_changed(Bitmap* new_use, Bitmap* old_use);
    void on_update_image();
    void on_transform_matrix_changed();
};

} // namespace glaxnimate::model
