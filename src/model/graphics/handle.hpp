#pragma once

#include <memory>
#include <QGraphicsObject>

namespace model::graphics {

class MoveHandle : public QGraphicsObject
{
Q_OBJECT

public:
    enum Direction
    {
        Horizontal,
        Vertical,
        DiagonalUp,
        DiagonalDown,
        Any,
    };

    enum Shape
    {
        Square,
        Diamond,
    };


    MoveHandle(
        QGraphicsItem* parent,
        Direction direction,
        Shape shape,
        int radius = 6,
        const QColor& color_rest = QColor(255, 255, 255),
        const QColor& color_highlighted = QColor(255, 255, 128),
        const QColor& color_selected = QColor(255, 255, 64),
        const QColor& color_border = QColor(0, 0, 0)
    );

    ~MoveHandle();


    QRectF boundingRect() const override;

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent * event) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent * event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent * event) override;

signals:
    void dragged(const QPointF& p);
    void dragged_x(qreal x);
    void dragged_y(qreal y);
    void drag_finished();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace model::graphics
