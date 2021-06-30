#pragma once

#include <memory>
#include <vector>

#include "model/document.hpp"
#include "model/shapes/shape.hpp"
#include "model/assets/brush_style.hpp"

class QWidget;

namespace tools {
class Tool;
} // namespace tools

namespace glaxnimate::gui {

class DocumentEnvironment
{
public:
//    DocumentEnvironment();
    virtual ~DocumentEnvironment() = default;

    virtual model::Document* document() const = 0;

    virtual model::VisualNode* current_document_node() const = 0;
    virtual void set_current_document_node(model::VisualNode* node) = 0;

    virtual model::Composition* current_composition() const = 0;
    virtual void set_current_composition(model::Composition* comp) = 0;

    model::ShapeElement* current_shape() const;
    /**
     * @brief Returns the property to add shapes into (never null)
     */
    model::ShapeListProperty* current_shape_container() const;

    virtual QColor current_color() const = 0;
    virtual void set_current_color(const QColor& c) = 0;
    virtual QColor secondary_color() const = 0;
    virtual void set_secondary_color(const QColor& c) = 0;
    virtual QWidget* as_widget() = 0;
    virtual QPen current_pen_style() const = 0;
    virtual qreal current_zoom() const = 0;

    virtual model::BrushStyle* linked_brush_style(bool secondary) const = 0;

    virtual void switch_tool(tools::Tool* tool) = 0;
    virtual std::vector<model::VisualNode*> cleaned_selection() const = 0;

    void delete_selected();
};


} // namespace glaxnimate::gui
