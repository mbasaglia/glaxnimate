#pragma once

#include "base.hpp"

#include "model/shapes/fill.hpp"
#include "model/shapes/group.hpp"
#include "model/shapes/stroke.hpp"
#include "command/shape_commands.hpp"

#include "widgets/tools/shape_tool_widget.hpp"
#include "command/undo_macro_guard.hpp"


namespace tools {

class DrawToolBase : public Tool
{
public:
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
                      std::unique_ptr<model::ShapeElement> shape)
    {
        auto document = event.window->document();
        command::UndoMacroGuard macro(command_name, document);

        model::VisualNode* select = shape.get();

        QString name = document->get_best_name(shape.get(), shape->name.get());

        model::ShapeListProperty* prop = get_container(event.window);

        ShapeToolWidget* options = widget();
        int index = prop->size();
        if ( options->create_group() )
        {
            auto group = std::make_unique<model::Group>(document);
            select = group.get();
            document->set_best_name(group.get(), QObject::tr("%1 Group").arg(name));
            auto super_prop = prop;
            prop = &group->shapes;

            model::VisualNode* owner_node = static_cast<model::VisualNode*>(super_prop->object());
            QTransform parent_t = owner_node->transform_matrix(owner_node->time());
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
            document->set_best_name(fill.get(), QObject::tr("Fill"));
            fill->color.set(event.window->current_color());
            fill->use.set(event.window->linked_brush_style(false));
            document->undo_stack().push(
                new command::AddShape(prop, std::move(fill), index)
            );
            index++;
        }

        if ( options->create_stroke() )
        {
            auto stroke = std::make_unique<model::Stroke>(document);
            document->set_best_name(stroke.get(), QObject::tr("Stroke"));
            stroke->set_pen_style(event.window->current_pen_style());
            stroke->use.set(event.window->linked_brush_style(true));

            document->undo_stack().push(
                new command::AddShape(prop, std::move(stroke), index)
            );
            index++;
        }

        shape->name.set(name);
        document->undo_stack().push(
            new command::AddShape(prop, std::move(shape), index)
        );

        event.window->set_current_document_node(select);
    }

    ShapeToolWidget* widget()
    {
        return static_cast<ShapeToolWidget*>(get_settings_widget());
    }

private:
    model::ShapeListProperty* get_container(GlaxnimateWindow* window)
    {
        return window->current_shape_container();
    }

};

} // namespace tools
