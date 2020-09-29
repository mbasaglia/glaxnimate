#pragma once

#include "base.hpp"

#include "model/shapes/fill.hpp"
#include "model/shapes/group.hpp"
#include "model/layers/shape_layer.hpp"
#include "model/shapes/stroke.hpp"
#include "command/layer_commands.hpp"
#include "command/shape_commands.hpp"

#include "widgets/tools/shape_tool_widget.hpp"


namespace tools {

class DrawToolBase : public Tool
{
public:
    app::settings::SettingList settings() const override { return {}; }

    QCursor cursor() override { return Qt::CrossCursor; }

protected:
    QWidget* on_create_widget() override
    {
        return new ShapeToolWidget();
    }

    void draw_shape(const PaintEvent& event, const QPainterPath& path)
    {
        ShapeToolWidget* options = widget();

        if ( !options->create_stroke() && !options->create_fill() )
        {
            event.painter->setBrush(Qt::transparent);

            //event.painter->setCompositionMode(QPainter::CompositionMode_Difference);
            QPen p(Qt::white, 1);
            p.setCosmetic(true);
            p.setDashPattern({4., 4.});
            event.painter->setPen(p);
            event.painter->drawPath(path);

            //event.painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
            p.setDashOffset(4);
            p.setColor(Qt::black);
            event.painter->setPen(p);
            event.painter->drawPath(path);
        }
        else
        {
            if ( options->create_fill() )
                event.painter->setBrush(event.window->current_color());
            else
                event.painter->setBrush(Qt::transparent);

            if ( options->create_stroke() )
            {
                QPen p = event.window->current_pen_style();
                p.setWidthF(p.widthF() * event.window->current_zoom());
                event.painter->setPen(p);
            }
            else
            {
                event.painter->setPen(Qt::NoPen);
            }

            event.painter->drawPath(path);
        }

    }

    void create_shape(const QString& command_name, const Event& event,
                      std::unique_ptr<model::Shape> shape)
    {
        auto document = event.window->document();
        document->undo_stack().beginMacro(command_name);

        model::DocumentNode* select = shape.get();

        QString name = document->get_best_name(shape.get());

        model::ShapeListProperty* prop = get_container(event.window);

        ShapeToolWidget* options = widget();
        int index = prop->index_of(event.window->current_shape()) + 1;
        if ( options->create_group() )
        {
            auto group = std::make_unique<model::Group>(document);
            select = group.get();
            document->set_best_name(group.get(), QObject::tr("%1 Group").arg(name));
            auto super_prop = prop;
            prop = &group->shapes;

            QTransform parent_t = super_prop->owner_node()->transform_matrix(super_prop->owner_node()->time());
            QTransform parent_t_inv = parent_t.inverted();
            group->transform.get()->set_transform_matrix(parent_t_inv);

            QPointF center = shape->local_bounding_rect(0).center();
            group->transform.get()->anchor_point.set(center);
            group->transform.get()->position.set(parent_t_inv.map(center));


            document->undo_stack().push(
                new command::AddShape(super_prop, std::move(group), index)
            );
            index = 0;
        }

        if ( options->create_fill() )
        {
            auto fill = std::make_unique<model::Fill>(document);
            document->set_best_name(fill.get(), QObject::tr("%1 Fill").arg(name));
            fill->color.set(event.window->current_color());
            document->undo_stack().push(
                new command::AddShape(prop, std::move(fill), index)
            );
            index++;
        }

        if ( options->create_stroke() )
        {
            auto stroke = std::make_unique<model::Stroke>(document);
            document->set_best_name(stroke.get(), QObject::tr("%1 Stroke").arg(name));
            stroke->color.set(event.window->secondary_color());
            QPen pen_style = event.window->current_pen_style();
            stroke->width.set(pen_style.width());
            stroke->cap.set(model::Stroke::Cap(pen_style.capStyle()));
            stroke->join.set(model::Stroke::Join(pen_style.joinStyle()));
            stroke->miter_limit.set(pen_style.miterLimit());

            document->undo_stack().push(
                new command::AddShape(prop, std::move(stroke), index)
            );
            index++;
        }

        shape->name.set(name);
        document->undo_stack().push(
            new command::AddShape(prop, std::move(shape), index)
        );

        document->undo_stack().endMacro();

        event.window->set_current_document_node(select);
    }

    ShapeToolWidget* widget()
    {
        return static_cast<ShapeToolWidget*>(get_settings_widget());
    }

    void check_click(const MouseEvent& event)
    {
        auto clicked_on = under_mouse(event, true);
        if ( !clicked_on.nodes.empty() )
        {
            auto tool = Registry::instance().tool("select");
            event.window->switch_tool(tool);
            tool->mouse_press(event);
            tool->mouse_release(event);
        }
    }

private:
    model::ShapeListProperty* get_container(GlaxnimateWindow* window)
    {
        if ( auto container = window->current_shape_container() )
            return container;

        auto comp = window->current_composition();
        for ( int i = comp->docnode_child_count() - 1; i >= 0; i-- )
        {
            if ( auto lay = qobject_cast<model::ShapeLayer*>(comp->docnode_child(i)) )
                return &lay->shapes;
        }

        auto document = window->document();
        auto layer = std::make_unique<model::ShapeLayer>(document, comp);
        int layer_index = comp->docnode_child_index(window->current_layer());
        if ( layer_index == -1 )
            layer_index = 0;

        QPointF center(
            document->main_composition()->width.get()/2,
            document->main_composition()->height.get()/2
        );
        layer->transform.get()->anchor_point.set(center);
        layer->transform.get()->position.set(center);
        document->set_best_name(layer.get());
        auto prop = &layer->shapes;
        document->undo_stack().push(
            new command::AddLayer(comp, std::move(layer), layer_index)
        );
        return prop;
    }

};

} // namespace tools
