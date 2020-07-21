#include "animation.hpp"
#include "model/graphics/animation_item.hpp"

model::graphics::DocumentNodeGraphicsItem * model::Animation::docnode_make_graphics_item()
{
    return new graphics::AnimationItem(this);
}

