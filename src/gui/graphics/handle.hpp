#pragma once

#include <memory>
#include <vector>

#include <QGraphicsObject>

#include "item_data.hpp"

namespace glaxnimate::model {

class AnimatableBase;

} // namespace glaxnimate::model

namespace glaxnimate::gui::graphics {

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
        Circle,
        Cross,
        Saltire,
        None,
    };

    enum HandleRole
    {
        NoHandle,
        Unknown,
        Vertex,
        Tangent,
        GradientStop,
        GradientHighlight,
    };

    MoveHandle(
        QGraphicsItem* parent,
        Direction direction,
        Shape shape,
        qreal radius = 6,
        bool dont_move = false,
        const QColor& color_rest = QColor(255, 255, 255),
        const QColor& color_highlighted = QColor(255, 255, 128),
        const QColor& color_selected = QColor(255, 255, 64),
        const QColor& color_border = QColor(0, 0, 0)
    );

    ~MoveHandle();

    void set_colors(
        const QColor& color_rest,
        const QColor& color_highlighted,
        const QColor& color_selected,
        const QColor& color_border
    );

    void change_shape(Shape shape, qreal radius=-1);
    void set_radius(qreal radius);

    QRectF boundingRect() const override;

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) override;

    HandleRole role() const;
    void set_role(HandleRole role);

    /**
     * \brief Set associated properties, shown on the context menu
     * \see set_associated_property(), clear_associated_properties()
     */
    void set_associated_properties(std::vector<model::AnimatableBase*> props);

    /**
     * \brief Set associated property, shown on the context menu
     * \see set_associated_properties(), clear_associated_properties()
     */
    void set_associated_property(model::AnimatableBase* prop);

    /**
     * \brief Clears associated properties
     * \see set_associated_properties(), set_associated_property()
     */
    void clear_associated_properties();

    void set_offset(const QPointF& offset);

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

} // namespace glaxnimate::gui::graphics
