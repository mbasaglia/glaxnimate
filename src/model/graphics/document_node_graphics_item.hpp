#pragma once

#include <QGraphicsObject>

namespace model {

class DocumentNode;

} // namespace model

namespace model::graphics {

class DocumentNodeGraphicsItem : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit DocumentNodeGraphicsItem(DocumentNode* node, QGraphicsItem* parent = nullptr)
        : QGraphicsObject(parent), node(node)
    {}

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;

public slots:
    void set_visible(bool v)
    {
        setVisible(v);
    }

signals:
    void focused(DocumentNode* document_node);

protected:
    void focusInEvent(QFocusEvent *) override
    {
        emit focused(node);
    }

private:
    DocumentNode* node;
};

} // namespace model::graphics
