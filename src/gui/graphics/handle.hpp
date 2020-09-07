#pragma once

#include <memory>
#include <QGraphicsObject>

namespace graphics {

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
        Rotate
    };

    enum Shape
    {
        Square,
        Diamond,
        Circle
    };


    MoveHandle(
        QGraphicsItem* parent,
        Direction direction,
        Shape shape,
        int radius = 6,
        bool dont_move = false,
        const QColor& color_rest = QColor(255, 255, 255),
        const QColor& color_highlighted = QColor(255, 255, 128),
        const QColor& color_selected = QColor(255, 255, 64),
        const QColor& color_border = QColor(0, 0, 0)
    );

    ~MoveHandle();

    void change_shape(Shape shape, int radius=-1);

    QRectF boundingRect() const override;

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent * event) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent * event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent * event) override;

signals:
    void drag_starting(const QPointF& p, Qt::KeyboardModifiers modifiers);
    void dragged(const QPointF& p, Qt::KeyboardModifiers modifiers);
    void dragged_x(qreal x, Qt::KeyboardModifiers modifiers);
    void dragged_y(qreal y, Qt::KeyboardModifiers modifiers);
    void drag_finished();
    void clicked(Qt::KeyboardModifiers modifiers);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace graphics
