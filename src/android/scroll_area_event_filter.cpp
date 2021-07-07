#include "scroll_area_event_filter.hpp"

#include <map>
#include <cmath>

#include <QMouseEvent>
#include <QScrollBar>
#include <QScroller>
#include <QEasingCurve>

class glaxnimate::android::ScrollAreaEventFilter::Private
{
public:
    QAbstractScrollArea *target;
    Qt::Orientations direction;
    QPointF scroll_start;
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
    {
        target->viewport()->installEventFilter(this);

        target->viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
        QScroller::grabGesture(target->viewport(), QScroller::TouchGesture);
        QScroller* scroller = QScroller::scroller(target);
        if ( !(d->direction & Qt::Horizontal) )
            scroller->setSnapPositionsX({0});
        if ( !(d->direction & Qt::Vertical) )
            scroller->setSnapPositionsY({0});

        QScrollerProperties prop = scroller->scrollerProperties();
        prop.setScrollMetric(QScrollerProperties::AxisLockThreshold, 0.66);
        prop.setScrollMetric(QScrollerProperties::ScrollingCurve, QEasingCurve(QEasingCurve::OutExpo));
        prop.setScrollMetric(QScrollerProperties::DecelerationFactor, 0.05);
        prop.setScrollMetric(QScrollerProperties::MaximumVelocity, 0.635);
        prop.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 0.33);
        prop.setScrollMetric(QScrollerProperties::OvershootScrollDistanceFactor, 0.33);
        prop.setScrollMetric(QScrollerProperties::SnapPositionRatio, 0.93);
        prop.setScrollMetric(QScrollerProperties::DragStartDistance, 0.001);
        scroller->setScrollerProperties(prop);
    }
}

bool glaxnimate::android::ScrollAreaEventFilter::eventFilter(QObject *object, QEvent *event)
{

    switch ( event->type() )
    {
        case QEvent::MouseButtonPress:
        {
            auto mouse_event = static_cast<QMouseEvent*>(event);
            d->scroll_start = mouse_event->pos();
            return true;
        }
        case QEvent::MouseButtonRelease:
        {
            auto mouse_event = static_cast<QMouseEvent*>(event);
            auto delta = mouse_event->pos() - d->scroll_start;
            if ( std::hypot(delta.x(), delta.y()) < 5 )
                emit clicked(mouse_event->pos());
            return true;
        }
        default:
            break;
    }

    return QObject::eventFilter(object, event);
}
