#include "document_scene.hpp"
#include "model/graphics/document_node_graphics_item.hpp"

class model::graphics::DocumentScene::Private
{
public:
    static constexpr int editor_z = 1000;

    Document* document = nullptr;
    std::unordered_map<DocumentNode*, DocumentNodeGraphicsItem*> node_to_item;
    std::unordered_map<DocumentNode*, std::vector<std::unique_ptr<QGraphicsItem>>> node_to_editors;

};

model::graphics::DocumentScene::DocumentScene()
    : d(std::make_unique<Private>())
{
}

model::graphics::DocumentScene::~DocumentScene()
{
    clear_selection();
}

void model::graphics::DocumentScene::set_document ( model::Document* document )
{
    if ( d->document )
    {
        disconnect_node(d->document->animation());
    }

    clear_selection();
    clear();

    d->document = document;

    if ( d->document )
    {
        connect_node(d->document->animation());
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

    for ( DocumentNode* child : node->docnode_children() )
        connect_node(child);

    DocumentNodeGraphicsItem* parent = nullptr;
    if ( auto parent_node = node->docnode_parent() )
    {
        auto it = d->node_to_item.find(parent_node);
        if ( it != d->node_to_item.end() )
            parent = it->second;
        child->setZValue(parent_node->docnode_child_index(node));
    }

    if ( parent )
        child->setParentItem(parent);
    else
        addItem(child);
}

void model::graphics::DocumentScene::disconnect_node ( model::DocumentNode* node )
{
    disconnect(node, &model::DocumentNode::docnode_child_add_end, this, &DocumentScene::connect_node);
    disconnect(node, &model::DocumentNode::docnode_child_remove_end, this, &DocumentScene::disconnect_node);

    for ( DocumentNode* child : node->docnode_children() )
        disconnect_node(child);

    auto item = d->node_to_item.find(node);
    if ( item != d->node_to_item.end() )
    {
        removeItem(item->second);
        delete item->second;
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


void model::graphics::DocumentScene::add_selection(model::DocumentNode* node)
{
    if ( d->node_to_editors.find(node) != d->node_to_editors.end() )
        return;

    auto items = node->docnode_make_graphics_editor();
    for ( const auto& item : items )
    {
        item->setZValue(Private::editor_z);
        addItem(item.get());
    }

    d->node_to_editors.emplace(node, std::move(items));
}


void model::graphics::DocumentScene::remove_selection(model::DocumentNode* node)
{
    auto it = d->node_to_editors.find(node);
    if ( it != d->node_to_editors.end() )
    {
        d->node_to_editors.erase(it);
    }
}

void model::graphics::DocumentScene::clear_selection()
{
    d->node_to_editors.clear();
}







