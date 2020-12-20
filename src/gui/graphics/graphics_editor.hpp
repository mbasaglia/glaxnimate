#pragma once

#include <QGraphicsObject>

#include "model/document_node.hpp"

namespace graphics {

class GraphicsEditor : public QGraphicsObject
{
public:
    explicit GraphicsEditor(model::DocumentNode* node) : node(node)
    {
        connect(node, &model::DocumentNode::transform_matrix_changed,
                this, &GraphicsEditor::set_transform_matrix);
        setTransform(node->transform_matrix(node->time()));
    }

    ~GraphicsEditor(){}

    template<class T, class... Args>
    T* add_child(Args&&... args)
    {
        T* item = new T(std::forward<Args>(args)...);
        item->setParentItem(this);
        return item;
    }

    QRectF boundingRect() const override { return {}; }
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override {}

model::DocumentNode* node;
public slots:
    void set_transform_matrix(const QTransform& t)
    {
        setTransform(t);
    }
};

} // namespace graphics

