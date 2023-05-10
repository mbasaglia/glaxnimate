/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "asset.hpp"
#include "model/property/object_list_property.hpp"
#include "model/shapes/layer.hpp"

namespace glaxnimate::model {


class Composition : public VisualNode, public AssetBase
{
    GLAXNIMATE_OBJECT(Composition)

    GLAXNIMATE_PROPERTY_LIST(model::ShapeElement, shapes)

    GLAXNIMATE_SUBOBJECT(AnimationContainer, animation)

    //                  type    name    default  notify                       validate
    GLAXNIMATE_PROPERTY(float,  fps,         60, &Composition::fps_changed,     &Composition::validate_fps)
    GLAXNIMATE_PROPERTY(int,    width,      512, &Composition::width_changed,   &Composition::validate_nonzero, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(int,    height,     512, &Composition::height_changed,  &Composition::validate_nonzero, PropertyTraits::Visual)

    Q_PROPERTY(QSize size READ size)
    Q_PROPERTY(QRectF rect READ rect)

public:
    using VisualNode::VisualNode;

    utils::Range<Layer::ChildLayerIterator> top_level() const
    {
        return {
            Layer::ChildLayerIterator(&shapes, nullptr, 0),
            Layer::ChildLayerIterator(&shapes, nullptr, shapes.size())
        };
    }

    DocumentNode* docnode_child(int index) const override
    {
        return shapes[index];
    }

    int docnode_child_count() const override
    {
        return shapes.size();
    }

    int docnode_child_index(DocumentNode* dn) const override;

    QRectF local_bounding_rect(FrameTime t) const override;

    QIcon tree_icon() const override;
    QString type_name_human() const override;
    bool remove_if_unused(bool clean_lists) override;
    DocumentNode* docnode_parent() const override;
    QSize size() const { return {width.get(), height.get()}; }
    QRectF rect() const { return QRectF(QPointF(0, 0), size()); }

    Q_INVOKABLE QImage render_image(float time, QSize size = {}, const QColor& background = {}) const;
    Q_INVOKABLE QImage render_image() const;

signals:
    void fps_changed(float fps);
    void width_changed(int);
    void height_changed(int);

private:
    bool validate_nonzero(int size) const
    {
        return size > 0;
    }

    bool validate_out_point(int p) const
    {
        return p > 0;
    }

    bool validate_fps(float v) const
    {
        return v > 0;
    }
};


} // namespace glaxnimate::model

