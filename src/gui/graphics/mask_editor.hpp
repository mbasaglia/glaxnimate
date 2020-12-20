#include <QGraphicsObject>

#include "model/shapes/layer.hpp"
#include "model/document_node.hpp"

#include "graphics/create_items.hpp"
#include "graphics/graphics_editor.hpp"

namespace graphics {

class MaskEditor : public QGraphicsObject
{
public:
    MaskEditor(model::Layer* layer)
    : layer(layer)
    {
//         setFlag(ItemHasNoContents);
        connect(layer->mask.get(), &model::MaskSettings::lock_transform_changed, this, &MaskEditor::toggle_transform);
        toggle_transform(layer->mask->lock_transform.get());
        if ( layer->mask->has_mask() )
            connect_child(layer->mask->mask.get(), this);
    }

    ~MaskEditor()
    {
        children.clear();
    }

    void paint(QPainter * , const QStyleOptionGraphicsItem *, QWidget *) override {}
    QRectF boundingRect() const override { return {}; }

private:
    void toggle_transform(bool enabled)
    {
        if ( enabled )
            setTransform(layer->transform_matrix(layer->time()));
        else
            setTransform({});
    }

    void connect_child(model::DocumentNode* node, QGraphicsItem* parent_item)
    {
        auto editor = GraphicsItemFactory::instance().make_graphics_editor(node);
        editor->setParentItem(this);

        connect(node, &model::DocumentNode::docnode_child_remove_end, this, &MaskEditor::disconnect_child);
        connect(node, &model::DocumentNode::docnode_child_add_end, this, [editptr=editor.get(), this](model::DocumentNode* node){
            connect_child(node, editptr);
        });

        for ( const auto& child : node->docnode_children() )
            connect_child(child, editor.get());

        children.emplace(node, std::move(editor));
    }

    void disconnect_child(model::DocumentNode* node)
    {
        disconnect(node, nullptr, this, nullptr);

        for ( const auto& child : node->docnode_children() )
            disconnect_child(child);

        children.erase(node);
    }

    void mask_changed(model::ShapeElement* new_mask, model::ShapeElement* old_mask)
    {
        if ( old_mask )
            disconnect_child(old_mask);

        children.clear();

        if ( new_mask )
            connect_child(new_mask, this);
    }

    model::Layer* layer;
    std::map<model::DocumentNode*, std::unique_ptr<graphics::GraphicsEditor>> children;
};

} // namespace graphics
