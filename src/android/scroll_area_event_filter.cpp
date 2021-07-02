#include "scroll_area_event_filter.hpp"

#include <map>
#include <cmath>

#include <QMouseEvent>
#include <QScrollBar>

class glaxnimate::android::ScrollAreaEventFilter::Private
{
public:
    QAbstractScrollArea *target;
    Qt::Orientations direction;
    QPoint scroll_start{-1, -1};
    QPoint scroll_last{-1, -1};
};

glaxnimate::android::ScrollAreaEventFilter::ScrollAreaEventFilter(QAbstractScrollArea *target, Qt::Orientations direction)
    : d(std::make_unique<Private>())
{
    set_target(target);
    d->direction = direction;
}

glaxnimate::android::ScrollAreaEventFilter::~ScrollAreaEventFilter()
{

}

void glaxnimate::android::ScrollAreaEventFilter::set_target(QAbstractScrollArea *target)
{
    d->target = target;
    if ( target )
        target->installEventFilter(this);
}

bool glaxnimate::android::ScrollAreaEventFilter::eventFilter(QObject *object, QEvent *event)
{
    switch ( event->type() )
    {
        case QEvent::MouseButtonPress:
        {
            auto mouse_event = static_cast<QMouseEvent*>(event);
            d->scroll_start = d->scroll_last = mouse_event->pos();
            return true;
        }
        case QEvent::MouseMove:
        {
            auto mouse_event = static_cast<QMouseEvent*>(event);
            auto delta = d->scroll_last - mouse_event->pos();

            if ( d->direction & Qt::Vertical )
            {
                d->target->verticalScrollBar()->setValue(
                    d->target->verticalScrollBar()->value() + delta.y()
                );
            }

            if ( d->direction & Qt::Horizontal )
            {
                d->target->horizontalScrollBar()->setValue(
                    d->target->horizontalScrollBar()->value() + delta.y()
                );
            }
            d->scroll_last = mouse_event->pos();
            return true;
        }
        case QEvent::MouseButtonRelease:
        {
            auto mouse_event = static_cast<QMouseEvent*>(event);
            auto delta = mouse_event->pos() - d->scroll_start;
            if ( std::hypot(delta.x(), delta.y()) < 5 )
            {
                emit clicked(mouse_event->pos());
            }
            return true;
        }
        default:
            break;
    }

    return QObject::eventFilter(object, event);
}
