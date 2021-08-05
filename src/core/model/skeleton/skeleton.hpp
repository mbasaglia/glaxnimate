#pragma once

#include "bone.hpp"
#include "skin.hpp"
#include "model/assets/assets.hpp"
#include "model/shapes/shape.hpp"

namespace glaxnimate::model {

class Skeleton;


template<class T, class Derived>
class SkeletonListBase : public AssetList<T, Derived>
{
protected:
    using Ctor = SkeletonListBase;

public:
    using AssetList<T, Derived>::AssetList;

    Skeleton* skeleton() const { return skeleton_; }
    QIcon tree_icon() const override { return {}; }

private:
    Skeleton* skeleton_ = nullptr;
    friend Skeleton;
};

class BoneList : public SkeletonListBase<BoneItem, BoneList>
{
    GLAXNIMATE_OBJECT(BoneList)
    ASSET_LIST_CLASS(BoneItem)

public:
    QString type_name_human() const override { return tr("Bones"); }
};

class SkinList : public SkeletonListBase<Skin, SkinList>
{
    GLAXNIMATE_OBJECT(SkinList)
    ASSET_LIST_CLASS(Skin)

public:
    QString type_name_human() const override { return tr("Skins"); }
};


class Skeleton : public model::ShapeElement
{
    GLAXNIMATE_OBJECT(Skeleton)
    GLAXNIMATE_SUBOBJECT(BoneList, bones)
    GLAXNIMATE_SUBOBJECT(SkinList, skins)

public:
    Skeleton(model::Document* document);
    ~Skeleton();

    QIcon tree_icon() const override;
    QString type_name_human() const override { return tr("Skeleton"); }
    int docnode_child_count() const override { return 0; }
    DocumentNode* docnode_child(int) const override { return nullptr; }
    int docnode_child_index(DocumentNode*) const override { return -1; }
    QRectF local_bounding_rect(FrameTime t) const override;

    QPainterPath to_painter_path(FrameTime t) const override;
    std::unique_ptr<ShapeElement> to_path() const override;
    void add_shapes(FrameTime t, math::bezier::MultiBezier& bez, const QTransform& transform) const override;

    Q_INVOKABLE glaxnimate::model::Bone* add_bone();
    Q_INVOKABLE glaxnimate::model::Skin* add_skin();

protected:
    void on_paint(QPainter* painter, FrameTime t, PaintMode mode, model::Modifier*) const override;
    class Private;
    std::unique_ptr<Private> d;
    friend Bone;
    friend SkinSlot;
    friend SkinItemBase;

};

} // namespace glaxnimate::model
