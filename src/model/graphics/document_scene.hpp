#pragma once

#include <QGraphicsScene>

#include "model/document.hpp"

namespace model::graphics {

class DocumentScene : public QGraphicsScene
{
    Q_OBJECT
public:
    DocumentScene();
    ~DocumentScene();

    void set_document(Document* document);
    void clear_document() { set_document(nullptr); }

    void add_selection(DocumentNode* node);
    void remove_selection(DocumentNode* node);
    void clear_selection();

public slots:
    void focus_node(model::DocumentNode* node);

signals:
    void node_focused(model::DocumentNode* node);

private slots:
    void connect_node(model::DocumentNode* node);
    void disconnect_node(model::DocumentNode* node);
    void on_focused(DocumentNodeGraphicsItem* item);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace model::graphics
