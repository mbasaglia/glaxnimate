#include "glaxnimate_window_p.hpp"

#include <queue>

#include "command/layer_commands.hpp"
#include "app/settings/widget_builder.hpp"
#include "model/layers/solid_color_layer.hpp"
#include "model/layers/shape_layer.hpp"
#include "model/shapes/group.hpp"


model::Composition* GlaxnimateWindow::Private::current_composition()
{
    model::DocumentNode* curr = current_document_node();
    if ( curr )
    {
        if ( auto curr_comp = qobject_cast<model::Composition*>(curr) )
            return curr_comp;

        if ( auto curr_lay = qobject_cast<model::Layer*>(curr) )
            return curr_lay->composition();
    }
    return current_document->main_composition();
}

model::Layer* GlaxnimateWindow::Private::current_layer()
{
    model::DocumentNode* curr = current_document_node();
    if ( curr )
    {
        if ( auto curr_lay = qobject_cast<model::Layer*>(curr) )
            return curr_lay;
    }
    return nullptr;
}


model::ShapeElement* GlaxnimateWindow::Private::current_shape()
{
    model::DocumentNode* curr = current_document_node();
    if ( curr )
    {
        if ( auto curr_shape = qobject_cast<model::ShapeElement*>(curr) )
            return curr_shape;
    }
    return nullptr;
}

model::DocumentNode* GlaxnimateWindow::Private::current_shape_container()
{
    model::DocumentNode* sh = current_document_node();
    if ( qobject_cast<model::ShapeLayer*>(sh) )
        return sh;

    sh = qobject_cast<model::ShapeElement*>(sh);
    while ( sh )
    {
        sh = sh->docnode_parent();
        if ( qobject_cast<model::Group*>(sh) || qobject_cast<model::ShapeLayer*>(sh) )
            return sh;
    }
    return nullptr;
}

model::DocumentNode* GlaxnimateWindow::Private::current_document_node()
{
    return document_node_model.node(ui.view_document_node->currentIndex());
}

void GlaxnimateWindow::Private::set_current_document_node(model::DocumentNode* node)
{
    ui.view_document_node->setCurrentIndex(document_node_model.node_index(node));
}

void GlaxnimateWindow::Private::layer_new_impl(std::unique_ptr<model::Layer> layer)
{
    model::Composition* composition = current_composition();

    current_document->set_best_name(layer.get(), {});

    layer->last_frame.set(current_document->main_composition()->last_frame.get());
    QPointF pos(
        current_document->main_composition()->width.get() / 2.0,
        current_document->main_composition()->height.get() / 2.0
    );
    layer->transform.get()->anchor_point.set(pos);
    layer->transform.get()->position.set(pos);

    if ( auto scl = qobject_cast<model::SolidColorLayer*>(layer.get()) )
    {
        scl->color.set(ui.color_selector->current_color());
    }

    model::Layer* ptr = layer.get();

    int position = composition->layer_position(current_layer());
    current_document->add_command(new command::AddLayer(composition, std::move(layer), position));

    ui.view_document_node->setCurrentIndex(document_node_model.node_index(ptr));
}

void GlaxnimateWindow::Private::layer_delete()
{
    /// @todo Remove shapes / precompositions
    model::DocumentNode* curr = current_document_node();
    if ( !curr )
        return;

    if ( auto curr_lay = qobject_cast<model::Layer*>(curr) )
    {
        current_document->add_command(new command::RemoveLayer(curr_lay));
    }
}

namespace {

struct SelectionFetcher
{
    std::set<model::DocumentNode*> to_search;
    int draw_order = 0;
    std::vector<int> indices;
    std::vector<model::DocumentNode*> selection;

    void gather(model::DocumentNode* node)
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
            for ( auto ch : node->docnode_children() )
                gather(ch);
        }
    }
};

} // namespace

std::vector<model::DocumentNode *> GlaxnimateWindow::Private::cleaned_selection()
{
    SelectionFetcher fetcher;

    for ( const auto& index : ui.view_document_node->selectionModel()->selectedIndexes() )
    {
        model::DocumentNode* node = document_node_model.node(index);
        if ( !node )
            continue;
        fetcher.to_search.insert(node);
    }
    fetcher.indices.reserve(fetcher.to_search.size());
    fetcher.selection.reserve(fetcher.to_search.size());
    fetcher.gather(current_composition());
    return fetcher.selection;
}
