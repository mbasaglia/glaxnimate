#include "keyframe_transition_widget.hpp"
#include <QPaintEvent>
#include <QPainter>
#include <QMouseEvent>


class KeyframeTransitionWidget::Private
{
public:
    model::KeyframeTransition* target = nullptr;
    int selected_handle = 0;
    int highlighted_handle = 0;
    int handle_radius = 8;
    QPoint drag_start_mouse;
    QPoint drag_start_handle;

    const math::Vec2& point(int i) { return target->bezier().points()[i]; }

    double map_coord(double coord, double size)
    {
        return handle_radius + (size - 2 * handle_radius) * coord;
    }

    QPointF map_pt(const math::Vec2& p, int width, int height)
    {
        return QPointF(
            map_coord(p.x(), width),
            map_coord((1-p.y()), height)
        );
    }

    QPointF mapped_point(int i, int width, int height)
    {
        return map_pt(point(i), width, height);
    }

    double unmap_coord(double coord, double size)
    {
        return qBound(0., (coord - handle_radius) / (size - 2.0 * handle_radius), 1.);
    }

    math::Vec2 unmap_pt(const QPoint& p, int width, int height)
    {
        return math::Vec2{
            unmap_coord(p.x(), width),
            (1-unmap_coord(p.y(), height)),
        };
    }

    void move_handle(QPoint p, int width, int height)
    {
        if ( selected_handle == 1 )
            target->set_before_handle(unmap_pt(p, width, height));
        else if ( selected_handle == 2 )
            target->set_after_handle(unmap_pt(p, width, height));
    }
};


KeyframeTransitionWidget::KeyframeTransitionWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    setMouseTracking(true);
}

KeyframeTransitionWidget::~KeyframeTransitionWidget() = default;

void KeyframeTransitionWidget::set_target(model::KeyframeTransition* kft)
{
    d->target = kft;
    update();
}

void KeyframeTransitionWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    QRect center_rect = rect().adjusted(
        d->handle_radius,
        d->handle_radius,
        -d->handle_radius,
        -d->handle_radius
    );

    QPalette::ColorGroup group = isEnabled() && d->target ? QPalette::Active : QPalette::Disabled;
    painter.fillRect(rect(), palette().brush(group, QPalette::Window));
    painter.fillRect(center_rect, palette().brush(group, QPalette::Base));

    if ( d->target )
    {
        painter.setRenderHint(QPainter::Antialiasing);

        QBrush fg = palette().brush(group, QPalette::Text);
        int w = width();
        int h = height();
        std::array<QPointF, 4> p = {
            d->mapped_point(0, w, h),
            d->mapped_point(1, w, h),
            d->mapped_point(2, w, h),
            d->mapped_point(3, w, h)
        };

        // Path
        QPainterPath pp(p[0]);
        pp.cubicTo(p[1], p[2], p[3]);
        painter.setBrush(Qt::transparent);
        painter.setPen(QPen(palette().brush(group, QPalette::Mid), 3));
        painter.drawPath(pp);
        painter.setPen(QPen(fg, 1));
        painter.drawPath(pp);

        // Connect handles
        QPen high_pen(palette().brush(group, QPalette::Highlight), 2);
        QPen low_pen(QPen(palette().brush(group, QPalette::Text), 2));
        painter.setBrush(Qt::transparent);
        painter.setPen(1 == d->selected_handle && isEnabled() ? high_pen : low_pen);
        painter.drawLine(p[0], p[1]);
        painter.setPen(2 == d->selected_handle && isEnabled() ? high_pen : low_pen);
        painter.drawLine(p[2], p[3]);

        // Fixed handles
        painter.setBrush(fg);
        painter.setPen(QPen(Qt::transparent));
        painter.drawEllipse(p[0], d->handle_radius, d->handle_radius);
        painter.drawEllipse(p[3], d->handle_radius, d->handle_radius);

        // Draggable handles
        for ( int i = 1; i <= 2; i++ )
        {
            if ( i == d->selected_handle && isEnabled() )
            {
                painter.setBrush(palette().brush(group, QPalette::Highlight));
                painter.setPen(Qt::transparent);
            }
            else if ( i == d->highlighted_handle && isEnabled() )
            {
                painter.setBrush(palette().brush(group, QPalette::HighlightedText));
                painter.setPen(QPen(palette().brush(group, QPalette::Highlight), 1));
            }
            else
            {
                painter.setBrush(palette().brush(group, QPalette::Base));
                painter.setPen(QPen(palette().brush(group, QPalette::Text), 1));
            }
            painter.drawEllipse(p[i], d->handle_radius, d->handle_radius);
        }
    }

}


void KeyframeTransitionWidget::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);
    if ( !d->target )
        return;

    if ( event->button() == Qt::LeftButton )
    {
        d->selected_handle = d->highlighted_handle;
        if ( d->selected_handle )
        {
            d->drag_start_mouse = event->pos();
            d->drag_start_handle = d->map_pt(d->point(d->selected_handle), width(), height()).toPoint();
        }
        update();
    }
}

void KeyframeTransitionWidget::mouseMoveEvent(QMouseEvent* event)
{
    QWidget::mouseMoveEvent(event);
    if ( !d->target )
        return;

    if ( d->selected_handle )
    {
        QPoint pos = event->pos() - d->drag_start_mouse + d->drag_start_handle;
        d->move_handle(pos, width(), height());
        update();
    }
    else if ( isEnabled() )
    {
        math::Vec2 p = d->unmap_pt(event->pos(), width(), height());
        double d1 = (d->point(1) - p).length_squared();
        double d2 = (d->point(2) - p).length_squared();
        d->highlighted_handle = d1 < d2 ? 1 : 2;
        update();
    }
}

void KeyframeTransitionWidget::mouseReleaseEvent(QMouseEvent* event)
{
    QWidget::mouseReleaseEvent(event);
    if ( !d->target )
        return;

    if ( event->button() == Qt::LeftButton )
    {
        d->selected_handle = 0;
        update();
    }
}

void KeyframeTransitionWidget::focusInEvent(QFocusEvent* event)
{
    QWidget::focusInEvent(event);
}

void KeyframeTransitionWidget::focusOutEvent(QFocusEvent* event)
{
    QWidget::focusOutEvent(event);
    d->selected_handle = 0;
    update();
}


void KeyframeTransitionWidget::leaveEvent(QEvent* event)
{
    QWidget::leaveEvent(event);
    if ( !d->selected_handle )
    {
        d->highlighted_handle = 0;
        update();
    }
}







