#include "document_scene.hpp"

#include <unordered_set>

#include "graphics/document_node_graphics_item.hpp"
#include "graphics/create_items.hpp"
#include "tools/base.hpp"

class graphics::DocumentScene::Private
{
public:
    static constexpr int editor_z = 1000;
    static constexpr int data_key_ptr = 0;

    using EditorMap = std::unordered_map<model::DocumentNode*, std::vector<std::unique_ptr<QGraphicsItem>>>;

    void remove_selection(model::DocumentNode* node)
    {
        auto it = node_to_editors.find(node);
        if ( it != node_to_editors.end() )
            node_to_editors.erase(it);

        auto it2 = selection_find(node);
        if ( it2 != selection.end() )
            selection.erase(it2);
    }

    model::DocumentNode* item_to_node(const QGraphicsItem* item) const
    {
        return item->data(data_key_ptr).value<model::DocumentNode*>();
    }


    std::vector<graphics::DocumentNodeGraphicsItem*> items_to_nodes(const QList<QGraphicsItem*>& items) const
    {
        std::vector<DocumentNodeGraphicsItem*> nodes;
        for ( auto item : items )
        {
            if ( item_to_node(item) )
                nodes.push_back(static_cast<DocumentNodeGraphicsItem*>(item));
        }
        return nodes;
    }

    std::vector<model::DocumentNode*>::iterator selection_find(model::DocumentNode* node)
    {
        return std::find(selection.begin(), selection.end(), node);
    }


    model::Document* document = nullptr;
    std::unordered_map<model::DocumentNode*, DocumentNodeGraphicsItem*> node_to_item;
    EditorMap node_to_editors;
    GraphicsItemFactory item_factory;
    std::vector<model::DocumentNode*> selection;
    tools::Tool* tool = nullptr;
};

graphics::DocumentScene::DocumentScene()
    : d(std::make_unique<Private>())
{
}

graphics::DocumentScene::~DocumentScene()
{
    clear_selection();
}

void graphics::DocumentScene::set_document ( model::Document* document )
{
    if ( d->document )
    {
        disconnect_node(d->document->main_composition());
    }

    clear_selection();
    clear();

    d->document = document;

    if ( d->document )
    {
        connect_node(d->document->main_composition());
    }
}

void graphics::DocumentScene::connect_node ( model::DocumentNode* node )
{
    DocumentNodeGraphicsItem* child = d->item_factory.make_graphics_item(node);
    if ( !child )
        return;

    d->node_to_item[node] = child;
    child->setData(Private::data_key_ptr, QVariant::fromValue(node));
    connect(node, &model::DocumentNode::docnode_child_add_end, this, &DocumentScene::connect_node);
    connect(node, &model::DocumentNode::docnode_child_remove_end, this, &DocumentScene::disconnect_node);
    connect(node, &model::DocumentNode::docnode_visible_changed, child, &DocumentNodeGraphicsItem::set_visible);

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

    for ( model::DocumentNode* child : node->docnode_children() )
        connect_node(child);
}

void graphics::DocumentScene::disconnect_node ( model::DocumentNode* node )
{
    disconnect(node, &model::DocumentNode::docnode_child_add_end, this, &DocumentScene::connect_node);
    disconnect(node, &model::DocumentNode::docnode_child_remove_end, this, &DocumentScene::disconnect_node);

    for ( model::DocumentNode* child : node->docnode_children() )
        disconnect_node(child);

    d->remove_selection(node);

    auto item = d->node_to_item.find(node);
    if ( item != d->node_to_item.end() )
    {
        removeItem(item->second);
        delete item->second;
        d->node_to_item.erase(item);
    }
}

void graphics::DocumentScene::add_selection(model::DocumentNode* node)
{
    auto it = d->node_to_item.find(node);
    if ( it != d->node_to_item.end() )
        it->second->setSelected(true);

    if ( d->selection_find(node) != d->selection.end() )
        return;

    d->selection.push_back(node);
    if ( d->tool && d->tool->show_editors(node) )
        show_editors(node);
}


void graphics::DocumentScene::remove_selection(model::DocumentNode* node)
{
    d->remove_selection(node);
}

void graphics::DocumentScene::toggle_selection(model::DocumentNode* node)
{
    if ( is_selected(node) )
        remove_selection(node);
    else
        add_selection(node);
}


void graphics::DocumentScene::clear_selection()
{
    d->node_to_editors.clear();
    d->selection.clear();
}

model::DocumentNode* graphics::DocumentScene::item_to_node(const QGraphicsItem* item) const
{
    return d->item_to_node(item);
}

void graphics::DocumentScene::user_select(const std::vector<model::DocumentNode *>& nodes, SelectMode flags)
{
    // Sorted ranges so we can use set operations
    std::vector<model::DocumentNode*> old_selection = d->selection;
    std::sort(old_selection.begin(), old_selection.end());
    std::vector<model::DocumentNode*> subject = nodes;
    std::sort(subject.begin(), subject.end());
    auto guess_size = std::max(old_selection.size(), subject.size());

    // In selection but not in nodes
    std::vector<model::DocumentNode*> selected_not_subject;
    selected_not_subject.reserve(guess_size);
    std::set_difference(
        old_selection.begin(), old_selection.end(),
        subject.begin(), subject.end(),
        std::inserter(selected_not_subject, selected_not_subject.end())
    );

    // In nodes but not in selection
    std::vector<model::DocumentNode*> subject_not_selected;
    subject_not_selected.reserve(guess_size);
    std::set_difference(
        subject.begin(), subject.end(),
        old_selection.begin(), old_selection.end(),
        std::inserter(subject_not_selected, subject_not_selected.end())
    );

    // In both
    std::vector<model::DocumentNode*> intersection;
    intersection.reserve(guess_size);
    std::set_intersection(
        old_selection.begin(), old_selection.end(),
        subject.begin(), subject.end(),
        std::inserter(intersection, intersection.end())
    );

    // Determine which chunks need to be added or removed to the selection
    std::vector<model::DocumentNode*> empty;
    std::vector<model::DocumentNode*>* selected = &empty;
    std::vector<model::DocumentNode*>* deselected = &empty;

    switch ( flags )
    {
        case Replace:
            // selected not subject -> deselect
            deselected = &selected_not_subject;
            // intersection -> unchanged
            // new nodes -> select
            selected = &subject_not_selected;
            break;
        case Append:
            // selected not subject -> unchanged
            // intersection -> unchanged
            // new nodes -> select
            selected = &subject_not_selected;
            deselected = &empty;
            break;
        case Toggle:
            // selected not subject -> unchanged
            // intersection -> deselect
            deselected = &intersection;
            // new nodes -> select
            selected = &subject_not_selected;
            break;
        case Remove:
            // selected not subject -> unchanged
            // intersection -> deselect
            deselected = &intersection;
            selected = &empty;
            // new nodes -> unchanged
            break;
    }

    for ( auto it = d->node_to_editors.begin(); it != d->node_to_editors.end(); )
    {
        if ( std::binary_search(deselected->begin(), deselected->end(), it->first) )
            it = d->node_to_editors.erase(it);
        else
            ++it;
    }

    d->selection.erase(
        std::remove_if(d->selection.begin(), d->selection.end(),
            [&deselected](model::DocumentNode* node){
                return std::binary_search(deselected->begin(), deselected->end(), node);
        }),
        d->selection.end()
    );

    for ( auto new_sel : *selected )
        add_selection(new_sel);

    emit node_user_selected(*selected, *deselected);

}

std::vector<graphics::DocumentNodeGraphicsItem*> graphics::DocumentScene::nodes(const QPointF& point, const QTransform& device_transform) const
{
    return d->items_to_nodes(items(point, Qt::IntersectsItemShape, Qt::DescendingOrder, device_transform));
}

std::vector<graphics::DocumentNodeGraphicsItem*> graphics::DocumentScene::nodes(const QPainterPath& path, const QTransform& device_transform) const
{
    return d->items_to_nodes(items(path, Qt::IntersectsItemShape, Qt::DescendingOrder, device_transform));
}

std::vector<graphics::DocumentNodeGraphicsItem*> graphics::DocumentScene::nodes(const QPolygonF& path, const QTransform& device_transform) const
{
    return d->items_to_nodes(items(path, Qt::IntersectsItemShape, Qt::DescendingOrder, device_transform));
}

bool graphics::DocumentScene::is_selected(model::DocumentNode* node) const
{
    return d->selection_find(node) != d->selection.end();
}

const std::vector<model::DocumentNode *> & graphics::DocumentScene::selection() const
{
    return d->selection;
}

void graphics::DocumentScene::show_editors(model::DocumentNode* node)
{
    auto items = d->item_factory.make_graphics_editor(node);
    for ( const auto& item : items )
    {
        item->setZValue(Private::editor_z);
        addItem(item.get());
    }

    d->node_to_editors.emplace(node, std::move(items));
}

void graphics::DocumentScene::set_active_tool(tools::Tool* tool)
{
    d->tool = tool;

    for ( auto node : d->selection )
    {
        if ( tool->show_editors(node) && !d->node_to_editors.count(node) )
            show_editors(node);
    }

    for ( auto it = d->node_to_editors.begin(); it != d->node_to_editors.end(); )
    {
        if ( !tool->show_editors(it->first) )
            it = d->node_to_editors.erase(it);
        else
            ++it;
    }

}