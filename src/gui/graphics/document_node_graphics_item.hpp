#pragma once

#include <QGraphicsObject>

namespace model {

class DocumentNode;
class BaseProperty;

} // namespace model

namespace graphics {

class DocumentNodeGraphicsItem : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit DocumentNodeGraphicsItem(model::DocumentNode* node, QGraphicsItem* parent = nullptr);
    ~DocumentNodeGraphicsItem();

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;

    model::DocumentNode* node() const
    {
        return node_;
    }

public slots:
    void set_visible(bool v)
    {
        setVisible(v);
    }

    void shape_changed();

    void set_transform_matrix(const QTransform& t)
    {
        setTransform(t);
    }

    void set_opacity(qreal op)
    {
        setOpacity(op);
    }

private slots:
    void on_property_changed(const model::BaseProperty* prop);

private:
    model::DocumentNode* node_;
};

} // namespace graphics
