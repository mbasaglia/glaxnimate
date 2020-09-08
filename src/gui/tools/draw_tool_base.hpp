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
                event.painter->setPen(event.window->current_pen_style());
            else
                event.painter->setPen(Qt::NoPen);

            event.painter->drawPath(path);
        }

    }

    void create_shape(const QString& command_name, const Event& event, std::unique_ptr<model::Shape> shape)
    {
        auto document = event.window->document();
        document->undo_stack().beginMacro(command_name);

        model::DocumentNode* select = shape.get();

        QString name = event.window->get_best_name(shape.get());

        auto container = event.window->current_shape_container();
        model::ShapeListProperty* prop = nullptr;
        if ( container )
        {
            prop = static_cast<model::ShapeListProperty*>(container->get_property("shapes"));
        }
        else
        {
            auto comp = event.window->current_composition();
            auto layer = std::make_unique<model::ShapeLayer>(document, comp);
            int layer_index = comp->docnode_child_index(event.window->current_layer());
            if ( layer_index == -1 )
                layer_index = 0;
            prop = &layer->shapes;
            event.window->set_best_name(layer.get());
            document->undo_stack().push(
                new command::AddLayer(comp, std::move(layer), layer_index)
            );
        }

        ShapeToolWidget* options = widget();
        int index = prop->index_of(event.window->current_shape(), 0);
        if ( options->create_group() )
        {
            auto group = std::make_unique<model::Group>(document);
            select = group.get();
            event.window->set_best_name(group.get(), QObject::tr("%1 Group").arg(name));
            auto super_prop = prop;
            prop = &group->shapes;
            document->undo_stack().push(
                new command::AddShape(super_prop, std::move(group), index)
            );
            index = 0;
        }

        if ( options->create_fill() )
        {
            auto fill = std::make_unique<model::SolidFill>(document);
            event.window->set_best_name(fill.get(), QObject::tr("%1 Fill").arg(name));
            fill->color.set(event.window->current_color());
            document->undo_stack().push(
                new command::AddShape(prop, std::move(fill), index)
            );
            index++;
        }

        if ( options->create_stroke() )
        {
            auto stroke = std::make_unique<model::SolidStroke>(document);
            event.window->set_best_name(stroke.get(), QObject::tr("%1 Stroke").arg(name));
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

};

} // namespace tools
