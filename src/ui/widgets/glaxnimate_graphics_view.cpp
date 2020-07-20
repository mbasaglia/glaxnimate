#include "glaxnimate_graphics_view.hpp"
#include <cmath>
#include <QMouseEvent>
#include <QtMath>

class GlaxnimateGraphicsView::Private
{
public:
    enum MouseMode
    {
        None,
    };

    enum MouseViewMode
    {
        NoDrag,
        Pan,
        Scale,
        Rotate
    };

    Private(GlaxnimateGraphicsView* view) : view(view) {}

    GlaxnimateGraphicsView* view;

    qreal zoom_factor = 1;
    qreal rotation = 0;

//     MouseMode mouse_mode = None;

    MouseViewMode mouse_view_mode = NoDrag;
    QPoint move_center;
    QPointF move_center_scene;

    QPoint transform_center;
    QPointF transform_center_scene;
    qreal scale_start_zoom = 1;


    void expand_scene_rect(float margin)
    {
        QRectF vp ( view->mapToScene(-margin,-margin),
                    view->mapToScene(view->width()+2*margin,view->height()+2*margin));
        QRectF sr = view->sceneRect();
        if ( ! sr.contains(vp) )
        {
            view->setSceneRect(sr.united(vp));
        }

//         emit scene_rect_changed(QRectF(mapToScene(0,0),mapToScene(width(),height())));
    }

    void update_mouse_cursor()
    {
        view->setCursor(Qt::ArrowCursor);
    }
};


GlaxnimateGraphicsView::GlaxnimateGraphicsView(QWidget* parent)
    : QGraphicsView(parent), d(std::make_unique<Private>(this))
{
    setMouseTracking(true);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(NoAnchor);
    setResizeAnchor(NoAnchor);
}

GlaxnimateGraphicsView::~GlaxnimateGraphicsView() = default;

void GlaxnimateGraphicsView::mousePressEvent(QMouseEvent* event)
{
    QPoint mpos = event->pos();
    QPointF scene_pos = mapToScene(mpos);

    if ( event->button() == Qt::MiddleButton )
    {
        if ( event->modifiers() & Qt::ControlModifier )
        {
            d->mouse_view_mode = Private::Scale;
            setCursor(Qt::SizeAllCursor);
            d->transform_center = mpos;
            d->scale_start_zoom = d->zoom_factor;
        }
        else if ( event->modifiers() & Qt::ShiftModifier )
        {
            d->mouse_view_mode = Private::Rotate;
            setCursor(Qt::SizeAllCursor);
            d->transform_center = mpos;
            d->transform_center_scene = scene_pos;
        }
        else
        {
            d->mouse_view_mode = Private::Pan;
            setCursor(Qt::ClosedHandCursor);
            d->transform_center_scene = scene_pos;
        }
    }

    d->move_center = mpos;
    d->move_center_scene = scene_pos;
}

void GlaxnimateGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    QPoint mpos = event->pos();
    QPointF scene_pos = mapToScene(mpos);

    if ( event->buttons() & Qt::MiddleButton  )
    {

        if ( d->mouse_view_mode == Private::Pan )
        {
            QPointF delta = scene_pos - d->transform_center_scene;
            translate_view(delta);
        }
        else if ( d->mouse_view_mode == Private::Scale )
        {
            QPointF delta = mpos - d->transform_center;
            qreal delta_x = delta.y();
            qreal factor = std::pow(5, delta_x/256);
            set_zoom(factor * d->scale_start_zoom, d->transform_center);
        }
        else if ( d->mouse_view_mode == Private::Rotate )
        {
            QPointF delta = mpos - d->transform_center;
            qreal len = std::hypot(delta.x(), delta.y());
            if ( len > 4 )
            {
                qreal ang = std::atan2(delta.y(), delta.x());
                QPointF deltap = d->move_center - d->transform_center;
                qreal angp = std::atan2(deltap.y(), deltap.x());
                do_rotate(ang-angp, d->transform_center_scene);
            }
        }
    }


    d->move_center = mpos;
    d->move_center_scene = scene_pos;
    scene()->invalidate();
}



void GlaxnimateGraphicsView::mouseReleaseEvent(QMouseEvent * event)
{
//     QPoint mpos = event->pos();
//     QPointF scene_pos = mapToScene(mpos);


    if ( event->button() == Qt::MiddleButton )
    {
        d->mouse_view_mode = Private::NoDrag;
    }

    d->update_mouse_cursor();

}

void GlaxnimateGraphicsView::wheelEvent(QWheelEvent* event)
{
    if ( event->delta() < 0 )
        zoom_view(0.8);
    else
        zoom_view(1.25);
}


void GlaxnimateGraphicsView::translate_view(const QPointF& delta)
{
    translate(delta);
    d->expand_scene_rect(10);
}

void GlaxnimateGraphicsView::zoom_view(qreal factor)
{
    zoom_view(factor, mapFromGlobal(QCursor::pos()));
}

void GlaxnimateGraphicsView::zoom_view(qreal factor, const QPoint& anchor)
{
    if ( d->zoom_factor*factor < 0.01 )
        return;

    QPointF sp1 = mapToScene(anchor);

    QRectF r ( mapToScene(0,0), mapToScene(width()/factor,height()/factor));
    r.translate(-r.bottomRight()/2);
    setSceneRect(sceneRect().united(r));

    scale(factor, factor);

    // Anchor
    if ( rect().contains(anchor) )
    {
        QPointF sp2 = mapToScene(anchor);
        translate(sp2-sp1);

    }
    d->expand_scene_rect(0);

    d->zoom_factor *= factor;
    emit zoomed(100*d->zoom_factor);

}

void GlaxnimateGraphicsView::set_zoom(qreal factor, const QPoint& anchor)
{
    if ( factor < 0.01 )
        return;
    zoom_view(factor / d->zoom_factor, anchor);
}

qreal GlaxnimateGraphicsView::get_zoom_factor() const
{
    return d->zoom_factor;
}

void GlaxnimateGraphicsView::paintEvent(QPaintEvent *event)
{
    QGraphicsView::paintEvent(event);

    if ( d->mouse_view_mode == Private::Rotate || d->mouse_view_mode == Private::Scale )
    {
        QPainter painter;
        QPoint p1 = d->move_center;
        QPoint p2 = d->transform_center;
        painter.begin(viewport());
        painter.setRenderHints(renderHints());
        painter.setPen(QPen(QColor(150, 150, 150), 3));
        painter.drawLine(p1, p2);
        painter.setPen(QPen(QColor(50, 50, 50), 1));
        painter.drawLine(p1, p2);
        painter.end();
    }
}

void GlaxnimateGraphicsView::do_rotate(qreal radians, const QPointF& scene_anchor)
{
    translate(scene_anchor);
    rotate(qRadiansToDegrees(radians));
    translate(-scene_anchor);
    d->expand_scene_rect(10);
    d->rotation += radians;
    emit rotated(qRadiansToDegrees(d->rotation));
}
