#pragma once

#include <QGraphicsScene>

#include "model/document.hpp"

namespace model::graphics {

class DocumentScene : public QGraphicsScene
{
    Q_OBJECT
public:
    enum SelectFlags
    {
        Replace = 0x00, ///< Replace current selection
        Append  = 0x01, ///< Append to current selection
        Toggle  = 0x02, ///< Toggles from selection
    };
    
    DocumentScene();
    ~DocumentScene();

    void set_document(Document* document);
    void clear_document() { set_document(nullptr); }

    void add_selection(DocumentNode* node);
    void remove_selection(DocumentNode* node);
    void toggle_selection(DocumentNode* node);
    void clear_selection();

    void user_select(const std::vector<model::DocumentNode*>& selected, SelectFlags flags);

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

} // namespace model::graphics

Q_DECLARE_METATYPE(std::vector<model::DocumentNode*>)
