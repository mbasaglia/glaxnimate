#pragma once

#include <QGraphicsScene>

#include "model/document.hpp"

namespace tools { class Tool; }

namespace graphics {

class DocumentNodeGraphicsItem;

class DocumentScene : public QGraphicsScene
{
    Q_OBJECT
public:
    enum SelectMode
    {
        Replace, ///< Replace current selection
        Append , ///< Append to current selection
        Toggle , ///< Toggles from selection
        Remove , ///< Removes from selection
    };

    DocumentScene();
    ~DocumentScene();

    void set_document(model::Document* document);
    void clear_document() { set_document(nullptr); }
    void set_active_tool(tools::Tool* tool);

    void add_selection(model::DocumentNode* node);
    void remove_selection(model::DocumentNode* node);
    void toggle_selection(model::DocumentNode* node);
    void clear_selection();
    void user_select(const std::vector<model::DocumentNode*>& selected, SelectMode flags);

    /// Currently selected nodes
    const std::vector<model::DocumentNode*>& selection() const;
    /// Top-level items for selected nodes
    std::vector<model::DocumentNode*> cleaned_selection();


    bool is_selected(model::DocumentNode* node) const;
    bool is_descendant_of_selection(model::DocumentNode* node) const;
    void show_editors(model::DocumentNode* node);

    model::DocumentNode* item_to_node(const QGraphicsItem* item) const;

    std::vector<DocumentNodeGraphicsItem*> nodes(const QPointF& point, const QTransform& device_transform) const;
    std::vector<DocumentNodeGraphicsItem*> nodes(const QPainterPath& path, const QTransform& device_transform) const;
    std::vector<DocumentNodeGraphicsItem*> nodes(const QPolygonF& path, const QTransform& device_transform) const;

signals:
    void node_user_selected(const std::vector<model::DocumentNode*>& selected, const std::vector<model::DocumentNode*>& deselected);

private slots:
    void connect_node(model::DocumentNode* node);
    void disconnect_node(model::DocumentNode* node);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace graphics
