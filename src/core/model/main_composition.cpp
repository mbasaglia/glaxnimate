#include "main_composition.hpp"
#include "model/graphics/main_composition_item.hpp"

GLAXNIMATE_OBJECT_IMPL(model::MainComposition)

model::graphics::DocumentNodeGraphicsItem * model::MainComposition::docnode_make_graphics_item()
{
    return new graphics::MainCompositionItem(this);
}

std::vector<std::unique_ptr<QGraphicsItem>> model::MainComposition::docnode_make_graphics_editor()
{
    std::vector<std::unique_ptr<QGraphicsItem>> v;
    v.push_back(std::make_unique<graphics::MainCompositionTransformItem>(this));
    return v;
}
