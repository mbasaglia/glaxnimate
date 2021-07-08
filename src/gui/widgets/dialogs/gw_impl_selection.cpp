#include "glaxnimate_window_p.hpp"
#include "widgets/shape_style/shape_style_preview_widget.hpp"



void GlaxnimateWindow::Private::scene_selection_changed(const std::vector<model::VisualNode*>& selected, const std::vector<model::VisualNode*>& deselected)
{
    selection_changed(selected, deselected);

    if ( !selected.empty() )
    {
        set_current_object(selected.back());
    }
    else
    {
        auto current = ui.view_document_node->currentIndex();
        if ( current.isValid() && ! ui.view_document_node->selectionModel()->isSelected(current) )
            set_current_object(nullptr);
    }
}

void GlaxnimateWindow::Private::timeline_current_node_changed(model::VisualNode* node)
{
    set_current_object(node);
}

void GlaxnimateWindow::Private::set_current_document_node(model::VisualNode* node)
{
    ui.view_document_node->replace_selection(node);
    ui.view_document_node->set_current_node(node);
}

void GlaxnimateWindow::Private::set_current_object(model::DocumentNode* node)
{
    if ( update_current || update_selection )
        return;

    auto lock = update_current.get_lock();

    model::Stroke* stroke = nullptr;
    model::Fill* fill = nullptr;

    if ( node )
    {
        stroke = qobject_cast<model::Stroke*>(node);
        fill = qobject_cast<model::Fill*>(node);
        if ( !stroke && !fill )
        {
            auto group = qobject_cast<model::Group*>(node);

            if ( !group )
            {
                if ( auto parent = node->docnode_parent() )
                    group = qobject_cast<model::Group*>(parent);
            }

            if ( group )
            {
                int stroke_count = 0;
                int fill_count = 0;
                for ( const auto& shape : group->shapes )
                {
                    if ( auto s = qobject_cast<model::Stroke*>(shape.get()) )
                    {
                        stroke = s;
                        stroke_count++;
                    }
                    else if ( auto f = qobject_cast<model::Fill*>(shape.get()) )
                    {
                        fill = f;
                        fill_count++;
                    }
                }

                if ( stroke_count > 1 )
                    stroke = nullptr;

                if ( fill_count > 1 )
                    fill = nullptr;
            }
        }
    }

    // Property view
    property_model.set_object(node);
    ui.view_properties->expandAll();

    // Timeline Widget
    if ( parent->sender() != ui.timeline_widget )
        ui.timeline_widget->set_current_node(node);

    // Document tree view
    ui.view_document_node->set_current_node(node);
    ui.view_document_node->repaint();

    // Styles
    ui.stroke_style_widget->set_shape(stroke);
    ui.fill_style_widget->set_shape(fill);
    ui.widget_gradients->set_targets(fill, stroke);
    widget_current_style->clear_gradients();

    if ( fill )
    {
        set_brush_reference(fill->use.get(), false);
        if ( !fill->visible.get() )
            widget_current_style->set_fill_color(Qt::transparent);
    }

    if ( stroke )
    {
        set_brush_reference(stroke->use.get(), true);
        if ( !stroke->visible.get() )
            widget_current_style->set_stroke_color(Qt::transparent);
    }
}


void GlaxnimateWindow::Private::selection_changed(const std::vector<model::VisualNode*>& selected, const std::vector<model::VisualNode*>& deselected)
{
    if ( update_selection )
        return;

    auto lock = update_selection.get_lock();

    if ( parent->sender() != ui.view_document_node && parent->sender() != ui.view_document_node->selectionModel() )
        ui.view_document_node->update_selection(selected, deselected);

    if ( parent->sender() != &scene )
    {
        scene.user_select(deselected, graphics::DocumentScene::Remove);
        scene.user_select(selected, graphics::DocumentScene::Append);
    }

    if ( parent->sender() != ui.timeline_widget )
        ui.timeline_widget->select(selected, deselected);

}
