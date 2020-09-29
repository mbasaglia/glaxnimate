#pragma once

#include "layer.hpp"

#include "model/shapes/shape.hpp"

namespace model {

class ShapeLayer : public detail::BaseLayerProps<ShapeLayer>
{
    GLAXNIMATE_OBJECT

public:
    ShapeListProperty shapes{this, "shapes",
        &DocumentNode::docnode_child_add_end,
        &DocumentNode::docnode_child_remove_end,
        &DocumentNode::docnode_child_add_begin,
        &DocumentNode::docnode_child_remove_begin
    };

    GLAXNIMATE_PROPERTY_LIST_IMPL(shapes)

public:
    using Ctor::Ctor;

    int docnode_child_count() const override { return shapes.size(); }
    DocumentNode* docnode_child(int index) const override { return shapes[index]; }
    int docnode_child_index(DocumentNode* obj) const override { return shapes.index_of(static_cast<ShapeElement*>(obj)); }

    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("shapes");
    }

    QString type_name_human() const override { return tr("Shape Layer"); }

    QRectF local_bounding_rect(model::FrameTime t) const override;
};


} // namespace model
