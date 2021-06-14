#pragma once

#include "shape.hpp"

#include "model/transform.hpp"
#include "model/property/sub_object_property.hpp"
#include "utils/range.hpp"

namespace model {


class Group : public StaticOverrides<Group, ShapeElement>
{
    GLAXNIMATE_OBJECT(Group)

public:
    GLAXNIMATE_PROPERTY_LIST(model::ShapeElement, shapes,
        &DocumentNode::docnode_child_add_end,
        &DocumentNode::docnode_child_remove_end,
        &DocumentNode::docnode_child_add_begin,
        &DocumentNode::docnode_child_remove_begin
    )
    GLAXNIMATE_SUBOBJECT(model::Transform, transform)
    GLAXNIMATE_ANIMATABLE(float, opacity, 1, &Group::opacity_changed, 0, 1, false, PropertyTraits::Percent)

public:
    Group(Document* document);

    int docnode_child_count() const override { return shapes.size(); }
    DocumentNode* docnode_child(int index) const override { return shapes[index]; }
    int docnode_child_index(DocumentNode* obj) const override { return shapes.index_of(static_cast<ShapeElement*>(obj)); }

    static QIcon static_tree_icon()
    {
        return QIcon::fromTheme("object-group");
    }

    static QString static_type_name_human()
    {
        return tr("Group");
    }

    void add_shapes(model::FrameTime t, math::bezier::MultiBezier & bez, const QTransform& transform) const override;

    QRectF local_bounding_rect(FrameTime t) const override;

    QTransform local_transform_matrix(model::FrameTime t) const override;

    QPainterPath to_clip(model::FrameTime t) const override;
    QPainterPath to_painter_path(model::FrameTime t) const override;

    std::unique_ptr<ShapeElement> to_path() const override;

signals:
    void opacity_changed(float op);

protected:
    void on_paint(QPainter*, FrameTime, PaintMode) const override;

private slots:
    void on_transform_matrix_changed();
};

} // namespace model

