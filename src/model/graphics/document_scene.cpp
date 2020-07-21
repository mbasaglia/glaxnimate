#include "document_scene.hpp"
#include "model/graphics/document_node_graphics_item.hpp"

class model::graphics::DocumentScene::Private
{
public:
    Document* document = nullptr;
    QHash<DocumentNode*, DocumentNodeGraphicsItem*> node_to_item;
};

model::graphics::DocumentScene::DocumentScene()
    : d(std::make_unique<Private>())
{
}

model::graphics::DocumentScene::~DocumentScene() = default;

void model::graphics::DocumentScene::set_document ( model::Document* document )
{
    if ( d->document )
    {
        disconnect_node(&d->document->animation());
    }

    d->document = document;

    if ( d->document )
    {
        connect_node(&d->document->animation());
    }
}

void model::graphics::DocumentScene::connect_node ( model::DocumentNode* node )
{
    DocumentNodeGraphicsItem* child = node->docnode_make_graphics_item();
    if ( !child )
        return;

    child->setData(0, QVariant::fromValue(node));
    connect(node, &model::DocumentNode::docnode_child_add_end, this, &DocumentScene::connect_node);
    connect(node, &model::DocumentNode::docnode_child_remove_end, this, &DocumentScene::disconnect_node);
    connect(node, &model::DocumentNode::docnode_visible_changed, child, &DocumentNodeGraphicsItem::set_visible);

    for ( int i = 0; i < node->docnode_child_count(); i++ )
        connect_node(node->docnode_child(i));

    DocumentNodeGraphicsItem* parent = d->node_to_item[node->docnode_parent()];
    if ( parent )
        child->setParentItem(parent);
    else
        addItem(child);
}

void model::graphics::DocumentScene::disconnect_node ( model::DocumentNode* node )
{
    disconnect(node, &model::DocumentNode::docnode_child_add_end, this, &DocumentScene::connect_node);
    disconnect(node, &model::DocumentNode::docnode_child_remove_end, this, &DocumentScene::disconnect_node);

    for ( int i = 0; i < node->docnode_child_count(); i++ )
        disconnect_node(node->docnode_child(i));

    auto item = d->node_to_item.find(node);
    if ( item != d->node_to_item.end() )
    {
        removeItem(*item);
        delete *item;
        d->node_to_item.erase(item);
    }
}

void model::graphics::DocumentScene::focus_node ( model::DocumentNode* node )
{
    DocumentNodeGraphicsItem* item = d->node_to_item[node];
    if ( !item || item == item->focusItem() )
        return;

    item->setFocus();
}

void model::graphics::DocumentScene::on_focused ( model::graphics::DocumentNodeGraphicsItem* item )
{
    if ( auto node = item->data(0).value<DocumentNode*>() )
        emit node_focused(node);
}




