#pragma once

#include "shape.hpp"

#include "model/transform.hpp"
#include "utils/range.hpp"

namespace model {


class Group : public Shape
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
    GLAXNIMATE_SUBOBJECT(Transform, transform)
    GLAXNIMATE_ANIMATABLE(float, opacity, 1)

public:
    using Shape::Shape;

    int docnode_child_count() const override { return shapes.size(); }
    DocumentNode* docnode_child(int index) const override { return &shapes[index]; }
    int docnode_child_index(DocumentNode* obj) const override { return shapes.index_of(static_cast<ShapeElement*>(obj)); }
    bool docnode_selection_container() const override { return true; }

    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("draw-rectangle");
    }

    QString type_name_human() const override
    {
        return tr("Group");
    }

    math::Bezier to_bezier(FrameTime) const override
    {
        return {};
    }

    void add_shapes(model::FrameTime t, math::MultiBezier & bez) const override
    {
        for ( const auto& ch : utils::Range(shapes.begin(), shapes.past_first_modifier()) )
        {
            ch->add_shapes(t, bez);
        }
    }

    QRectF local_bounding_rect(FrameTime t) const override
    {
        QRectF rect;
        for ( const auto& ch : utils::Range(shapes.begin(), shapes.past_first_modifier()) )
        {
            if ( rect.isNull() )
                rect = ch->local_bounding_rect(t);
            else
                rect |= ch->local_bounding_rect(t);
        }

        return rect;
    }

// signals:
//     void shape_added(ShapeElement* layer);
//     void shape_removed(ShapeElement* layer);
};

} // namespace model

