#pragma once

#pragma once

#include "model/composition.hpp"
#include "model/document.hpp"
#include "graphics/document_node_graphics_item.hpp"

namespace graphics {

class CompositionItem : public DocumentNodeGraphicsItem
{
public:
    explicit CompositionItem (model::Composition* animation)
        : DocumentNodeGraphicsItem(animation)
    {
        setFlag(QGraphicsItem::ItemIsSelectable, false);
        setFlag(QGraphicsItem::ItemHasNoContents, false);
        set_selection_mode(None);
        connect(animation->document(), &model::Document::graphics_invalidated, this, [this]{refresh();});
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override
    {
        node()->paint(painter, node()->time(), model::VisualNode::Canvas);
    }

    void refresh()
    {
        update();
    }

private slots:
    void size_changed()
    {
        prepareGeometryChange();
    }
};

} // namespace graphics


