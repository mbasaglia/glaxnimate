#pragma once

#include <memory>
#include <vector>

#include <QObject>

#include "model/document.hpp"
#include "model/shapes/shape.hpp"
#include "model/assets/brush_style.hpp"
#include "model/assets/precomposition.hpp"

#include "io/mime/mime_serializer.hpp"

class QWidget;

namespace tools {
class Tool;
} // namespace tools

namespace glaxnimate::gui {

class SelectionManager
{
public:
//    SelectionManager();
    virtual ~SelectionManager() = default;

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

    std::vector<model::VisualNode*> copy() const;
    void cut();
    void paste();
    void paste_as_composition();
    void paste_document(model::Document* document, const QString& macro_name, bool as_comp);
    void delete_shapes();

    virtual void set_selection(const std::vector<model::VisualNode*>& selected) = 0;
    virtual void update_selection(const std::vector<model::VisualNode*>& selected, const std::vector<model::VisualNode*>& deselected) = 0;

protected:
    virtual std::vector<io::mime::MimeSerializer*> supported_mimes() const = 0;
    void layer_new_impl(std::unique_ptr<model::ShapeElement> layer);
    model::PreCompLayer *layer_new_comp(model::Precomposition *comp);

private:
    void paste_impl(bool as_comp);
    void delete_shapes_impl(const QString& undo_string, const std::vector<model::VisualNode *> &selection);
};


} // namespace glaxnimate::gui
