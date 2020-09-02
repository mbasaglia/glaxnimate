#pragma once

#include "layer.hpp"

#include "model/shape.hpp"

namespace model {


class ShapeLayer : public detail::BaseLayerProps<ShapeLayer>
{
    GLAXNIMATE_OBJECT

    GLAXNIMATE_PROPERTY_LIST(ShapeElement, shapes,
        &ShapeLayer::shape_added,
        &ShapeLayer::shape_removed,
        &DocumentNode::docnode_child_add_begin,
        &DocumentNode::docnode_child_remove_begin)
public:
    // shapes

    using Ctor::Ctor;

    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("shapes");
    }

    QString type_name_human() const override { return tr("Shape Layer"); }

signals:
    void shape_added(ShapeElement* layer);
    void shape_removed(ShapeElement* layer);
};


} // namespace model
