#pragma once

#include "bone.hpp"
#include "model/assets/assets.hpp"
#include "model/shapes/shape.hpp"

namespace glaxnimate::model {

class Skeleton;


template<class T, class Derived>
class SkeletonListBase : public AssetListBase<T, Derived>
{
protected:
    using Ctor = SkeletonListBase;

public:
    using AssetListBase<T, Derived>::AssetListBase;


private:
    Skeleton* parent = nullptr;
    friend Skeleton;
};

class BoneList : public AssetListBase<BoneItem, BoneList>
{
    GLAXNIMATE_OBJECT(BoneList)
    ASSET_LIST_CLASS(Bone)

public:
    QIcon tree_icon() const override;
    QString type_name_human() const override { return tr("Bones"); }
};


class Skeleton : public model::ShapeElement
{
    GLAXNIMATE_OBJECT(Skeleton)
    GLAXNIMATE_SUBOBJECT(BoneList, bones)

public:
    using ShapeElement::ShapeElement;

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

protected:
    void on_paint(QPainter* painter, FrameTime t, PaintMode mode, model::Modifier*) const override;
};

} // namespace glaxnimate::model
