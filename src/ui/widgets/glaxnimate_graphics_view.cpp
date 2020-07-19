#include "glaxnimate_graphics_view.hpp"
#include <cmath>
#include <QMouseEvent>

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

//     MouseMode mouse_mode = None;

    MouseViewMode mouse_view_mode = NoDrag;
    QPoint move_center;
    QPoint transform_center;
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
    setResizeAnchor(AnchorViewCenter);
}

GlaxnimateGraphicsView::~GlaxnimateGraphicsView() = default;

void GlaxnimateGraphicsView::mousePressEvent(QMouseEvent* event)
{
    QPoint mpos = event->pos();
//     QPointF scene_pos = mapToScene(mpos);

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
        }
        else
        {
            d->mouse_view_mode = Private::Pan;
            setCursor(Qt::ClosedHandCursor);
        }
    }

    d->move_center = mpos;
}

void GlaxnimateGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    QPoint mpos = event->pos();
//     QPointF scene_pos = mapToScene(mpos);

    if ( event->buttons() & Qt::MiddleButton  )
    {

        if ( d->mouse_view_mode == Private::Pan )
        {
            QPointF delta = mpos - d->move_center;
            delta /= d->zoom_factor; // take scaling into account
            translate_view(delta);
        }
        else if ( d->mouse_view_mode == Private::Scale )
        {
            QPoint delta = mpos - d->transform_center;
            qreal delta_x = delta.x();
            qreal factor = std::pow(5, delta_x/256);
            set_zoom(factor * d->scale_start_zoom, d->transform_center);
        }
        else if ( d->mouse_view_mode == Private::Rotate )
        {
        }
    }


    d->move_center = mpos;
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
    if ( event->modifiers() & Qt::ControlModifier )
    {
        if ( event->delta() < 0 )
            zoom_view(0.8);
        else
            zoom_view(1.25);
    }
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
