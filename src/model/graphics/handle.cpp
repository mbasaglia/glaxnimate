#include "handle.hpp"
#include <QPainter>
#include <QtMath>
#include <QCursor>

class model::graphics::MoveHandle::Private
{
public:
    Direction direction;
    Shape shape;
    qreal radius;
    QColor color_rest;
    QColor color_highlighted;
    QColor color_selected;
    QColor color_border;

    qreal external_radius()
    {
        return radius + 1;
    }
};


model::graphics::MoveHandle::MoveHandle(
    QGraphicsItem* parent,
    Direction direction,
    Shape shape,
    int radius,
    const QColor& color_rest,
    const QColor& color_highlighted,
    const QColor& color_selected,
    const QColor& color_border)
: QGraphicsObject(parent),
    d(std::make_unique<Private>(Private{direction, shape, qreal(radius),
        color_rest, color_highlighted, color_selected, color_border}))
{
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsFocusable);
    setFlag(QGraphicsItem::ItemIgnoresTransformations);
    setAcceptHoverEvents(true);

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

model::graphics::MoveHandle::~MoveHandle() = default;

QRectF model::graphics::MoveHandle::boundingRect() const
{
    return {-d->external_radius(), -d->external_radius(), d->external_radius()*2, d->external_radius()*2};
}

void model::graphics::MoveHandle::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->save();

    qreal radius = isSelected() ? d->external_radius() : d->radius;

    painter->setPen(QPen(d->color_border, 1));
    if ( isSelected() )
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
    }


    painter->restore();
}
