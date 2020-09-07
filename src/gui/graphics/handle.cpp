#include "handle.hpp"
#include <QPainter>
#include <QtMath>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>

class graphics::MoveHandle::Private
{
public:
    Direction direction;
    Shape shape;
    qreal radius;
    bool dont_move;
    QColor color_rest;
    QColor color_highlighted;
    QColor color_selected;
    QColor color_border;
    bool dragged = false;

    qreal external_radius()
    {
        return radius + 1;
    }

    void apply_constraints(const QPointF& reference, QPointF& constrained)
    {
        if ( direction == Horizontal )
            constrained.setY(reference.y());
        else if ( direction == Vertical )
            constrained.setX(reference.x());
    }
};


graphics::MoveHandle::MoveHandle(
    QGraphicsItem* parent,
    Direction direction,
    Shape shape,
    int radius,
    bool dont_move,
    const QColor& color_rest,
    const QColor& color_highlighted,
    const QColor& color_selected,
    const QColor& color_border)
: QGraphicsObject(parent),
    d(std::make_unique<Private>(Private{direction, shape, qreal(radius),
        dont_move, color_rest, color_highlighted, color_selected, color_border}))
{
    setFlag(QGraphicsItem::ItemIsFocusable);
    setFlag(QGraphicsItem::ItemIgnoresTransformations);
    setAcceptedMouseButtons(Qt::LeftButton);

    if ( d->direction == Horizontal )
        setCursor(Qt::SizeHorCursor);
    else if ( d->direction == Vertical )
        setCursor(Qt::SizeVerCursor);
    else if ( d->direction == DiagonalUp )
        setCursor(Qt::SizeBDiagCursor);
    else if ( d->direction == DiagonalDown )
        setCursor(Qt::SizeFDiagCursor);
    else
        setCursor(Qt::SizeAllCursor);
}

graphics::MoveHandle::~MoveHandle() = default;

QRectF graphics::MoveHandle::boundingRect() const
{
    return {-d->external_radius(), -d->external_radius(), d->external_radius()*2, d->external_radius()*2};
}

void graphics::MoveHandle::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->save();

    qreal radius = hasFocus() ? d->external_radius() : d->radius;

    painter->setPen(QPen(d->color_border, 1));
    if ( hasFocus() )
        painter->setBrush(d->color_selected);
    else if ( isUnderMouse() )
        painter->setBrush(d->color_highlighted);
    else
        painter->setBrush(d->color_rest);

    if ( d->shape == Diamond )
    {
        painter->rotate(45);
        radius /= M_SQRT2;
    }

    QRectF rect{-radius, -radius, radius*2, radius*2};

    switch ( d->shape )
    {
        case Square:
        case Diamond:
            painter->drawRect(rect);
            break;
        case Circle:
            painter->drawEllipse(rect);
            break;
    }

    painter->restore();
}

void graphics::MoveHandle::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    setFocus(Qt::MouseFocusReason);
    event->accept();
    d->dragged = false;
}

void graphics::MoveHandle::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if ( !d->dragged )
    {
        d->dragged = true;
        emit drag_starting(pos(), event->modifiers());
    }

    QTransform scene_to_parent = parentItem()->sceneTransform().inverted();
    QPointF oldp = scene_to_parent.map(scenePos());
    QPointF p = scene_to_parent.map(event->scenePos());
    d->apply_constraints(oldp, p);
    if ( !d->dont_move )
        setPos(p);
    event->accept();
    emit dragged(p, event->modifiers());
    emit dragged_x(p.x(), event->modifiers());
    emit dragged_y(p.y(), event->modifiers());

}

void graphics::MoveHandle::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    clearFocus();
    event->accept();
    if ( d->dragged )
    {
        emit drag_finished();
        d->dragged = false;
    }
    else
    {
        emit clicked(event->modifiers());
    }
}

void graphics::MoveHandle::change_shape(graphics::MoveHandle::Shape shape, int radius)
{
    d->shape = shape;
    if ( radius > 0 && radius != d->radius )
    {
        d->radius = radius;
        prepareGeometryChange();
    }
    update();
}
