#pragma once

#include "base.hpp"

#include "glaxnimate/core/model/shapes/fill.hpp"
#include "glaxnimate/core/model/shapes/group.hpp"
#include "glaxnimate/core/model/shapes/stroke.hpp"
#include "glaxnimate/core/command/shape_commands.hpp"

#include "widgets/tools/shape_tool_widget.hpp"
#include "glaxnimate/core/command/undo_macro_guard.hpp"


namespace glaxnimate::gui::tools {

class DrawToolBase : public Tool
{
public:
    QCursor cursor() override { return Qt::CrossCursor; }

protected:
    QWidget* on_create_widget() override
    {
        return new ShapeToolWidget();
    }

    void draw_shape(const PaintEvent& event, const QPainterPath& path);

    void create_shape(const QString& command_name, const Event& event,
                      std::unique_ptr<model::ShapeElement> shape);

    ShapeToolWidget* widget()
    {
        return static_cast<ShapeToolWidget*>(get_settings_widget());
    }


    void shape_style_change_event(const glaxnimate::gui::tools::Event & event) override;

private:

    model::ShapeListProperty* get_container(glaxnimate::gui::SelectionManager* window)
    {
        return window->current_shape_container();
    }

};

} // namespace glaxnimate::gui::tools
