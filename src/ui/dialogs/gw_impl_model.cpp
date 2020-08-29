#include "glaxnimate_window_p.hpp"

#include "command/layer_commands.hpp"
#include "app/settings/widget_builder.hpp"


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

model::DocumentNode* GlaxnimateWindow::Private::current_document_node()
{
    return document_node_model.node(ui.view_document_node->currentIndex());
}

void GlaxnimateWindow::Private::layer_new_impl(std::unique_ptr<model::Layer> layer)
{
    model::Composition* composition = current_composition();

    QString base_name = layer->type_name_human();
    QString name = base_name;

    int n = 0;
    for ( int i = 0; i < composition->layers.size(); i++ )
    {
        if ( composition->layers[i].name.get() == name )
        {
            n += 1;
            name = tr("%1 %2").arg(base_name).arg(n);
            i = -1;
        }
    }

    layer->name.set(name);

    layer->last_frame.set(current_document->main_composition()->last_frame.get());
    QPointF pos(
        current_document->main_composition()->width.get() / 2.0,
        current_document->main_composition()->height.get() / 2.0
    );
    layer->transform.get()->anchor_point.set(pos);
    layer->transform.get()->position.set(pos);

    if ( auto scl = qobject_cast<model::SolidColorLayer*>(layer.get()) )
    {
        scl->color.set(current_color());
    }

    model::Layer* ptr = layer.get();

    int position = composition->layer_position(current_layer());
    current_document->undo_stack().push(new command::AddLayer(composition, std::move(layer), position));

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
        current_document->undo_stack().push(new command::RemoveLayer(curr_lay->composition(), curr_lay));
    }
}

