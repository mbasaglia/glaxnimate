#pragma once

#include <vector>
#include <memory>

#include <QGraphicsItem>

namespace model {
    class DocumentNode;
} // namespace model

namespace graphics {

class DocumentNodeGraphicsItem;

DocumentNodeGraphicsItem* docnode_make_graphics_item(model::DocumentNode* node);

std::vector<std::unique_ptr<QGraphicsItem>> docnode_make_graphics_editor(model::DocumentNode* node);

} // namespace graphics
