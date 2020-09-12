#include "glaxnimate_window_p.hpp"

#include <queue>
#include <QClipboard>

#include "command/layer_commands.hpp"
#include "command/shape_commands.hpp"
#include "command/structure_commands.hpp"
#include "app/settings/widget_builder.hpp"
#include "model/layers/solid_color_layer.hpp"
#include "model/layers/shape_layer.hpp"
#include "model/shapes/group.hpp"
#include "misc/clipboard_settings.hpp"


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

model::ShapeListProperty* GlaxnimateWindow::Private::current_shape_container()
{
    model::DocumentNode* sh = current_document_node();
    if ( auto lay = qobject_cast<model::ShapeLayer*>(sh) )
        return &lay->shapes;

    sh = qobject_cast<model::ShapeElement*>(sh);
    while ( sh )
    {
        sh = sh->docnode_parent();
        if ( auto grp = qobject_cast<model::Group*>(sh) )
            return &grp->shapes;
        if ( auto lay = qobject_cast<model::ShapeLayer*>(sh) )
            return &lay->shapes;
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

void GlaxnimateWindow::Private::layer_new_prepare(model::Layer* layer)
{
    current_document->set_best_name(layer, {});

    layer->last_frame.set(current_document->main_composition()->last_frame.get());
    QPointF pos = current_document->rect().center();
    layer->transform.get()->anchor_point.set(pos);
    layer->transform.get()->position.set(pos);
    layer->set_time(current_document->current_time());
}

void GlaxnimateWindow::Private::layer_new_impl(std::unique_ptr<model::Layer> layer)
{
    model::Composition* composition = current_composition();

    layer_new_prepare(layer.get());

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

void GlaxnimateWindow::Private::cut()
{
    auto selection = copy();
    if ( selection.empty() )
        return;

    current_document->undo_stack().beginMacro(tr("Cut"));
    for ( auto item : selection )
        current_document->add_command(new command::DeleteCommand(item));
    current_document->undo_stack().endMacro();
}

std::vector<model::DocumentNode*> GlaxnimateWindow::Private::copy()
{
    auto selection = cleaned_selection();

    if ( !selection.empty() )
    {
        QMimeData* data = new QMimeData;
        for ( const auto& mime : ClipboardSettings::mime_types() )
        {
            if ( mime.enabled )
                mime.serializer->to_mime_data(*data, selection);
        }

        QGuiApplication::clipboard()->setMimeData(data);
    }

    return selection;
}

void GlaxnimateWindow::Private::paste()
{
    const QMimeData* data = QGuiApplication::clipboard()->mimeData();
    std::vector<std::unique_ptr<model::DocumentNode>> raw_pasted;
    for ( const auto& mime : ClipboardSettings::mime_types() )
    {
        if ( mime.enabled )
        {
            raw_pasted = mime.serializer->from_mime_data(*data, current_document.get(), current_composition());
            if ( !raw_pasted.empty() )
                break;
        }
    }
    if ( raw_pasted.empty() )
    {
        status_message(tr("Nothing to paste"));
        return;
    }

    std::vector<std::unique_ptr<model::Layer>> layers;
    std::vector<std::unique_ptr<model::ShapeElement>> shapes;
    /// \todo precompositions
    /// \todo document defs
    for ( auto& ptr : raw_pasted )
    {
        if ( auto main_comp = qobject_cast<model::MainComposition*>(ptr.get()) )
        {
            layers.reserve(layers.size() + main_comp->layers.size());
            auto raw = main_comp->layers.raw();
            layers.insert(layers.end(), raw.move_begin(), raw.move_end());
            raw.clear();
        }
        else if ( auto lay = qobject_cast<model::Layer*>(ptr.get()) )
        {
            ptr.release();
            layers.emplace_back(lay);
        }
        else if ( auto shape = qobject_cast<model::ShapeElement*>(ptr.get()) )
        {
            ptr.release();
            shapes.emplace_back(shape);
        }
    }
    raw_pasted.clear();

    current_document->undo_stack().beginMacro(tr("Paste"));

    model::Composition* composition = current_composition();
    model::ShapeListProperty* shape_cont = current_shape_container();
    int layer_insertion_point = 0;
    if ( model::Layer* curr_layer = current_layer() )
        layer_insertion_point = composition->layer_position(curr_layer, -1) + 1;
    else
        layer_insertion_point = composition->layers.size();

    std::vector<model::DocumentNode*> select;
    if ( !shapes.empty() )
    {
        if ( !shape_cont )
        {
            auto new_layer = std::make_unique<model::ShapeLayer>(current_document.get(), composition);
            layer_new_prepare(new_layer.get());
            shape_cont = &new_layer->shapes;
            select.push_back(new_layer.get());
            current_document->add_command(new command::AddLayer(composition, std::move(new_layer), layer_insertion_point++));
        }
        int shape_insertion_point = shape_cont->size();
        for ( auto& shape : shapes )
        {
            select.push_back(shape.get());
            shape->recursive_rename();
            current_document->add_command(new command::AddShape(shape_cont, std::move(shape), shape_insertion_point++));
        }
    }

    for ( auto& layer : layers )
    {
        select.push_back(layer.get());
        layer->recursive_rename();
        current_document->add_command(new command::AddLayer(composition, std::move(layer), layer_insertion_point++));
    }

    current_document->undo_stack().endMacro();

    QItemSelection item_select;
    for ( auto node : select )
    {
        item_select.push_back(QItemSelectionRange(document_node_model.node_index(node)));
    }
    ui.view_document_node->selectionModel()->select(item_select, QItemSelectionModel::ClearAndSelect|QItemSelectionModel::Rows);
}
