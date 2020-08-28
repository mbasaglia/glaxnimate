#include "animation.hpp"
#include "model/graphics/animation_item.hpp"

GLAXNIMATE_OBJECT_IMPL(model::Animation)

model::graphics::DocumentNodeGraphicsItem * model::Animation::docnode_make_graphics_item()
{
    return new graphics::AnimationItem(this);
}

std::vector<std::unique_ptr<QGraphicsItem>> model::Animation::docnode_make_graphics_editor()
{
    std::vector<std::unique_ptr<QGraphicsItem>> v;
    v.push_back(std::make_unique<graphics::AnimationTransformItem>(this));
    return v;
}
