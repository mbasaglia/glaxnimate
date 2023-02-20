/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "document_scene.hpp"

#include <QDebug>
#include <set>

#include "app/application.hpp"
#include "graphics/document_node_graphics_item.hpp"
#include "graphics/create_items.hpp"
#include "graphics/graphics_editor.hpp"
#include "graphics/item_data.hpp"
#include "tools/base.hpp"

using namespace glaxnimate::gui;

class graphics::DocumentScene::Private
{
public:
    static constexpr int editor_z = 1000;

    using EditorMap = std::unordered_map<model::VisualNode*, std::unique_ptr<QGraphicsItem>>;

    void remove_selection(model::VisualNode* node)
    {
        remove_editors(node, false, false);

        auto it2 = selection_find(node);
        if ( it2 != selection.end() )
            selection.erase(it2);
    }

    void remove_editors(model::VisualNode* node, bool recursive, bool check_selection)
    {
        auto it = node_to_editors.find(node);
        if ( it != node_to_editors.end() )
            node_to_editors.erase(it);

        if ( recursive )
        {
            for ( auto child: node->docnode_visual_children() )
                if ( !check_selection || selection_find(child) == selection.end() )
                    remove_editors(child, true, check_selection);
        }
    }

    void remove_selection_recursive(model::VisualNode* node)
    {
        remove_selection(node);
        for ( auto ch : node->docnode_visual_children() )
            remove_selection_recursive(ch);
    }

    model::VisualNode* item_to_node(const QGraphicsItem* item) const
    {
        return item->data(ItemData::NodePointer).value<model::VisualNode*>();
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

    std::vector<model::VisualNode*>::iterator selection_find(model::VisualNode* node)
    {
        return std::find(selection.begin(), selection.end(), node);
    }

    model::Composition* current_composition()
    {
        return comp;
    }

    DocumentNodeGraphicsItem* item_from_node(model::VisualNode* node)
    {
        auto it = node_to_item.find(node);
        if ( it == node_to_item.end() )
            return nullptr;
        return it->second;
    }

    model::Document* document = nullptr;
    std::unordered_map<model::VisualNode*, DocumentNodeGraphicsItem*> node_to_item;
    EditorMap node_to_editors;
    std::vector<model::VisualNode*> selection;
    tools::Tool* tool = nullptr;
    QBrush back;
    model::Composition* comp = nullptr;
    bool show_masks = false;
};

graphics::DocumentScene::DocumentScene()
    : d(std::make_unique<Private>())
{
    setItemIndexMethod(QGraphicsScene::ItemIndexMethod::NoIndex);
    d->back.setTexture(QPixmap(app::Application::instance()->data_file("images/widgets/background.png")));
}

graphics::DocumentScene::~DocumentScene()
{
    clear_selection();
}

glaxnimate::model::Document* graphics::DocumentScene::document() const
{
    return d->document;
}

void graphics::DocumentScene::set_document ( model::Document* document )
{
    auto old = d->document;

    d->document = document;
    set_composition(document ? document->main() : nullptr);
    if ( document )
    {
        connect(document, &model::Document::graphics_invalidated, this, [this]{update();});
        connect(document, &model::Document::record_to_keyframe_changed, this, [this]{update();});
    }

    emit document_changed(document, old);
}

void graphics::DocumentScene::set_composition(model::Composition* comp)
{
    if ( d->comp )
    {
        disconnect_node(d->comp);
    }

    clear_selection();
    clear();

    d->comp = comp;

    if ( d->comp )
    {
        connect_node(d->comp);
    }
}

void graphics::DocumentScene::connect_node ( model::DocumentNode* n )
{
    auto node = static_cast<model::VisualNode*>(n);
    DocumentNodeGraphicsItem* child = GraphicsItemFactory::instance().make_graphics_item(node);
    if ( !child )
        return;

    d->node_to_item[node] = child;
    child->setData(ItemData::NodePointer, QVariant::fromValue(node));
    connect(node, &model::DocumentNode::docnode_child_add_end, this, &DocumentScene::connect_node);
    connect(node, &model::DocumentNode::docnode_child_remove_end, this, &DocumentScene::disconnect_node);
    connect(node, &model::DocumentNode::docnode_child_move_end, this, &DocumentScene::move_node);
    connect(node, &model::VisualNode::docnode_locked_changed, this, &DocumentScene::node_locked);

    DocumentNodeGraphicsItem* parent = nullptr;
    if ( auto parent_node = node->docnode_visual_parent() )
    {
        auto it = d->node_to_item.find(parent_node);
        if ( it != d->node_to_item.end() )
        {
            parent = it->second;
            child->setParentItem(parent);
            int index = parent_node->docnode_child_index(node);
            if ( index < parent_node->docnode_child_count() - 1 )
                child->stackBefore(d->node_to_item.find(parent_node->docnode_visual_child(index+1))->second);
        }
    }

    if ( !parent )
        addItem(child);

    for ( auto i = node->docnode_child_count() - 1; i >= 0; i-- )
        connect_node(node->docnode_child(i));
}

void graphics::DocumentScene::disconnect_node ( model::DocumentNode* n )
{
    auto node = static_cast<model::VisualNode*>(n);

    disconnect(node, &model::DocumentNode::docnode_child_add_end, this, &DocumentScene::connect_node);
    disconnect(node, &model::DocumentNode::docnode_child_remove_end, this, &DocumentScene::disconnect_node);

    for ( model::DocumentNode* child : node->docnode_children() )
        disconnect_node(child);

    d->remove_selection_recursive(node);

    auto item = d->node_to_item.find(node);
    if ( item != d->node_to_item.end() )
    {
        delete item->second;
        d->node_to_item.erase(item);
    }
}

void graphics::DocumentScene::add_selection(model::VisualNode* node)
{
    auto it = d->node_to_item.find(node);
    if ( it == d->node_to_item.end() )
        return;

    it->second->setSelected(true);

    if ( d->selection_find(node) != d->selection.end() )
        return;

    d->selection.push_back(node);

    d->tool->on_selected(this, node);
}


void graphics::DocumentScene::remove_selection(model::VisualNode* node)
{
    d->remove_selection(node);
    d->tool->on_deselected(this, node);
}

void graphics::DocumentScene::toggle_selection(model::VisualNode* node)
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

glaxnimate::model::VisualNode* graphics::DocumentScene::item_to_node(const QGraphicsItem* item) const
{
    return d->item_to_node(item);
}

void graphics::DocumentScene::user_select(const std::vector<model::VisualNode*>& nodes, SelectMode flags)
{
    if ( nodes.empty() && flags != Replace )
        return;

    // Sorted ranges so we can use set operations
    std::vector<model::VisualNode*> old_selection = d->selection;
    std::sort(old_selection.begin(), old_selection.end());
    std::vector<model::VisualNode*> subject = nodes;
    std::sort(subject.begin(), subject.end());
    auto guess_size = std::max(old_selection.size(), subject.size());

    // In selection but not in nodes
    std::vector<model::VisualNode*> selected_not_subject;
    selected_not_subject.reserve(guess_size);
    std::set_difference(
        old_selection.begin(), old_selection.end(),
        subject.begin(), subject.end(),
        std::inserter(selected_not_subject, selected_not_subject.end())
    );

    // In nodes but not in selection
    std::vector<model::VisualNode*> subject_not_selected;
    subject_not_selected.reserve(guess_size);
    std::set_difference(
        subject.begin(), subject.end(),
        old_selection.begin(), old_selection.end(),
        std::inserter(subject_not_selected, subject_not_selected.end())
    );

    // In both
    std::vector<model::VisualNode*> intersection;
    intersection.reserve(guess_size);
    std::set_intersection(
        old_selection.begin(), old_selection.end(),
        subject.begin(), subject.end(),
        std::inserter(intersection, intersection.end())
    );

    // Determine which chunks need to be added or removed to the selection
    std::vector<model::VisualNode*> empty;
    std::vector<model::VisualNode*>* selected = &empty;
    std::vector<model::VisualNode*>* deselected = &empty;

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

    d->selection.erase(
        std::remove_if(d->selection.begin(), d->selection.end(),
            [&deselected](model::VisualNode* node){
                return std::binary_search(deselected->begin(), deselected->end(), node);
        }),
        d->selection.end()
    );

    for ( auto old_sel : *deselected )
    {
        d->node_to_editors.erase(old_sel);
        d->tool->on_deselected(this, old_sel);
    }

    for ( auto new_sel : *selected )
        add_selection(new_sel);

    if ( !selected->empty() || !deselected->empty() )
        emit node_user_selected(*selected, *deselected);

}

std::vector<graphics::DocumentNodeGraphicsItem*> graphics::DocumentScene::nodes(const QPointF& point, const QTransform& device_transform) const
{
    return d->items_to_nodes(items(point, Qt::IntersectsItemShape, Qt::DescendingOrder, device_transform));
}

std::vector<graphics::DocumentNodeGraphicsItem*> graphics::DocumentScene::nodes(const QPainterPath& path, const QTransform& device_transform, Qt::ItemSelectionMode mode) const
{
    return d->items_to_nodes(items(path, mode, Qt::DescendingOrder, device_transform));
}

std::vector<graphics::DocumentNodeGraphicsItem*> graphics::DocumentScene::nodes(const QPolygonF& path, const QTransform& device_transform, Qt::ItemSelectionMode mode) const
{
    return d->items_to_nodes(items(path, mode, Qt::DescendingOrder, device_transform));
}

bool graphics::DocumentScene::is_selected(model::VisualNode* node) const
{
    return d->selection_find(node) != d->selection.end();
}

bool graphics::DocumentScene::is_descendant_of_selection(model::VisualNode* node) const
{
    for ( ; node ; node = node->docnode_visual_parent() )
        if ( is_selected(node) )
            return true;
    return false;
}


namespace {

struct SelectionFetcher
{
    std::set<glaxnimate::model::VisualNode*> to_search;
    int draw_order = 0;
    std::vector<int> indices;
    std::vector<glaxnimate::model::VisualNode*> selection;

    void gather(glaxnimate::model::VisualNode* node)
    {
        if ( to_search.count(node) )
        {
            auto it = std::upper_bound(indices.begin(), indices.end(), draw_order);
            indices.insert(it, draw_order);
            selection.insert(selection.begin() + (it - indices.begin()), node);
            draw_order++;
        }
        else
        {
            for ( auto ch : node->docnode_visual_children() )
                gather(ch);
        }
    }
};

} // namespace

std::vector<glaxnimate::model::VisualNode *> graphics::DocumentScene::cleaned_selection()
{
    SelectionFetcher fetcher;
    fetcher.to_search.insert(d->selection.begin(), d->selection.end());
    fetcher.indices.reserve(fetcher.to_search.size());
    fetcher.selection.reserve(fetcher.to_search.size());
    fetcher.gather(d->current_composition());
    return fetcher.selection;
}

const std::vector<glaxnimate::model::VisualNode *> & graphics::DocumentScene::selection() const
{
    return d->selection;
}

void graphics::DocumentScene::show_editors(model::VisualNode* node)
{
    if ( !d->node_to_editors.count(node) )
    {
        if ( auto item = GraphicsItemFactory::instance().make_graphics_editor(node) )
        {
            item->setZValue(Private::editor_z);
            addItem(item.get());
            d->node_to_editors.emplace(node, std::move(item));
        }
    }
}

void graphics::DocumentScene::set_active_tool(tools::Tool* tool)
{
    d->tool = tool;
    d->node_to_editors.clear();
    for ( auto node : selection() )
        d->tool->on_selected(this, node);
}

void graphics::DocumentScene::move_node(model::DocumentNode* node, int, int)
{
    model::VisualNode* parent_node = static_cast<model::VisualNode*>(node)->docnode_visual_parent();
    int siblings_count = parent_node->docnode_child_count();

    QGraphicsItem* above = d->item_from_node(parent_node->docnode_visual_child(siblings_count - 1));
    for ( int i = siblings_count - 2; i >= 0; i-- )
    {
        QGraphicsItem* item = d->item_from_node(parent_node->docnode_visual_child(i));
        item->stackBefore(above);
        above = item;
    }

}

void graphics::DocumentScene::node_locked(bool locked)
{
    if ( locked )
    {
        model::VisualNode* node = qobject_cast<model::VisualNode*>(sender());
        d->remove_selection_recursive(node);
    }

}

void graphics::DocumentScene::drawBackground(QPainter* painter, const QRectF& rect)
{
    painter->fillRect(rect, palette().base());
    if ( d->document )
        painter->fillRect(rect.intersected(QRectF(QPointF(0, 0), d->document->size())), d->back);
    emit drawing_background(painter, rect);
}

void graphics::DocumentScene::drawForeground(QPainter* painter, const QRectF&)
{
    painter->setBrush(Qt::NoBrush);
    QPen p(palette().mid(), 1);
    p.setCosmetic(true);
    painter->setPen(p);
    if ( d->document )
        painter->drawRect(QRectF(QPointF(0, 0), d->document->size()));

}

void graphics::DocumentScene::hide_editors(model::VisualNode* node, bool recursive, bool if_not_selected)
{
    d->remove_editors(node, recursive, if_not_selected);
}


bool graphics::DocumentScene::has_editors(model::VisualNode* node) const
{
    return d->node_to_editors.find(node) != d->node_to_editors.end();
}

QGraphicsItem* graphics::DocumentScene::get_editor(model::VisualNode* node) const
{
    auto it = d->node_to_editors.find(node);
    if ( it == d->node_to_editors.end() )
        return nullptr;

    return it->second.get();
}

void graphics::DocumentScene::show_custom_editor(model::VisualNode* node, std::unique_ptr<QGraphicsItem> editor)
{
    auto it = d->node_to_editors.find(node);
    addItem(editor.get());
    if ( it != d->node_to_editors.end() )
        it->second = std::move(editor);
    else
        d->node_to_editors.emplace(node, std::move(editor));
}

graphics::DocumentNodeGraphicsItem* graphics::DocumentScene::item_from_node(model::VisualNode* node) const
{
    return d->item_from_node(node);
}

void graphics::DocumentScene::debug() const
{
    qDebug() << "== Scene ==";

    qDebug() << "Selection";
    for ( auto node : d->selection )
        qDebug() << node << node->object_name();

    qDebug() << "Node Items";
    for ( const auto& p : d->node_to_item )
        qDebug() << p.first << p.first->object_name() << p.second;

    qDebug() << "Editors";
    for ( const auto& p : d->node_to_editors )
        qDebug() << p.first << p.first->object_name() << p.second.get();

    qDebug() << "Items";
    for ( auto it : items() )
        qDebug() << it << it->parentItem();

    qDebug() << "==";
}
