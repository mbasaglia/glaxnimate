/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "shape.hpp"

#include "model/transform.hpp"
#include "model/property/sub_object_property.hpp"
#include "utils/range.hpp"

namespace glaxnimate::model {


class Group : public StaticOverrides<Group, ShapeElement>
{
    GLAXNIMATE_OBJECT(Group)

public:
    GLAXNIMATE_PROPERTY_LIST(ShapeElement, shapes)
    GLAXNIMATE_SUBOBJECT(Transform, transform)
    GLAXNIMATE_ANIMATABLE(float, opacity, 1, &Group::opacity_changed, 0, 1, false, PropertyTraits::Percent)
    GLAXNIMATE_PROPERTY(bool, auto_orient, false, &Group::on_transform_matrix_changed, {}, PropertyTraits::Visual|PropertyTraits::Hidden)

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

    std::unique_ptr<ShapeElement> to_path() const override;

Q_SIGNALS:
    void opacity_changed(float op);

protected:
    QPainterPath to_painter_path_impl(model::FrameTime t) const override;
    void on_paint(QPainter*, FrameTime, PaintMode, model::Modifier*) const override;
    void on_graphics_changed() override;
    void on_composition_changed(model::Composition* old_comp, model::Composition* new_comp) override;

private Q_SLOTS:
    void on_transform_matrix_changed();
};

} // namespace glaxnimate::model

