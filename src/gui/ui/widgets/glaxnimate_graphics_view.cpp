#include "glaxnimate_graphics_view.hpp"

#include <cmath>

#include <QMouseEvent>
#include <QtMath>


#include "tools/base.hpp"
#include "graphics/document_scene.hpp"


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

    tools::Tool* tool = nullptr;
    GlaxnimateWindow* tool_target = nullptr;

//     MouseMode mouse_mode = None;

    MouseViewMode mouse_view_mode = NoDrag;
    QPoint move_last;
    QPoint move_last_screen;
    QPointF move_last_scene;
    Qt::MouseButton press_button;
    QPoint move_press_screen;
    QPointF move_press_scene;

    QPoint transform_center;
    QPointF transform_center_scene;
    qreal scale_start_zoom = 1;


    void expand_scene_rect(float margin)
    {
        QRectF vp = QRectF(
            view->mapToScene(-margin,-margin),
            view->mapToScene(view->width()+margin, view->height()+margin)
        ).normalized();

        QRectF sr = view->sceneRect();
        if ( ! sr.contains(vp) )
        {
            view->setSceneRect(sr.united(vp));
        }

//         emit scene_rect_changed(QRectF(mapToScene(0,0),mapToScene(width(),height())));
    }

    void update_mouse_cursor()
    {
        if ( tool )
            view->setCursor(tool->cursor());
        else
            view->unsetCursor();
    }

    QPointF anchor_scene()
    {
        QPoint anchor = view->mapFromGlobal(QCursor::pos());
        QRect vp = view->rect();
        if ( !vp.contains(anchor) )
            anchor = vp.center();
        return view->mapToScene(anchor);
    }

    tools::MouseEvent mouse_event(QMouseEvent* ev)
    {
        return {
            {
                view,
                static_cast<graphics::DocumentScene*>(view->scene()),
                tool_target
            },
            ev,
            view->mapToScene(ev->pos()),
            press_button,
            move_press_scene,
            move_press_screen,
            move_last_scene,
            move_last_screen,
        };
    }

    tools::PaintEvent paint_event(QPainter* painter)
    {
        return {
            {
                view,
                static_cast<graphics::DocumentScene*>(view->scene()),
                tool_target
            },
            painter
        };
    }

    tools::KeyEvent key_event(QKeyEvent* ev)
    {
        return {
            {
                view,
                static_cast<graphics::DocumentScene*>(view->scene()),
                tool_target
            },
            ev
        };
    }
};


GlaxnimateGraphicsView::GlaxnimateGraphicsView(QWidget* parent)
    : QGraphicsView(parent), d(std::make_unique<Private>(this))
{
    setMouseTracking(true);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(NoAnchor);
    setResizeAnchor(NoAnchor);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

GlaxnimateGraphicsView::~GlaxnimateGraphicsView() = default;

void GlaxnimateGraphicsView::mousePressEvent(QMouseEvent* event)
{
//     QGraphicsView::mousePressEvent(event);

    QPoint mpos = event->pos();
    QPointF scene_pos = mapToScene(mpos);

    d->press_button = event->button();
    d->move_press_scene = scene_pos;
    d->move_press_screen = event->screenPos().toPoint();
    d->move_last = mpos;
    d->move_last_scene = scene_pos;
    d->move_last_screen = event->screenPos().toPoint();

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
            d->expand_scene_rect(std::max(width(), height())*3);
        }
        else
        {
            d->mouse_view_mode = Private::Pan;
            setCursor(Qt::ClosedHandCursor);
            d->transform_center_scene = scene_pos;
        }
    }
    else if ( d->tool )
    {
        d->tool->mouse_press(d->mouse_event(event));
    }
}

void GlaxnimateGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
//     QGraphicsView::mouseMoveEvent(event);

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
            set_zoom_anchor(factor * d->scale_start_zoom, d->transform_center_scene);
        }
        else if ( d->mouse_view_mode == Private::Rotate )
        {
            QPointF delta = mpos - d->transform_center;
            qreal len = std::hypot(delta.x(), delta.y());
            if ( len > 4 )
            {
                qreal ang = std::atan2(delta.y(), delta.x());
                QPointF deltap = d->move_last - d->transform_center;
                qreal angp = std::atan2(deltap.y(), deltap.x());
                do_rotate(ang-angp, d->transform_center_scene);
            }
        }
    }
    else if ( d->tool )
    {
        d->tool->mouse_move(d->mouse_event(event));
    }


    d->move_last = mpos;
    d->move_last_scene = scene_pos;
    d->move_last_screen = event->screenPos().toPoint();
//     scene()->invalidate();
    viewport()->update();
}



void GlaxnimateGraphicsView::mouseReleaseEvent(QMouseEvent * event)
{
//     QGraphicsView::mouseReleaseEvent(event);
//     QPoint mpos = event->pos();
//     QPointF scene_pos = mapToScene(mpos);


    if ( event->button() == Qt::MiddleButton )
    {
        d->mouse_view_mode = Private::NoDrag;
    }
    else if ( d->tool )
    {
        d->tool->mouse_release(d->mouse_event(event));
    }

    d->update_mouse_cursor();
}

void GlaxnimateGraphicsView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if ( d->tool )
    {
        d->tool->mouse_double_click(d->mouse_event(event));
    }
}

void GlaxnimateGraphicsView::keyPressEvent(QKeyEvent* event)
{
    if ( d->tool )
    {
        d->tool->key_press(d->key_event(event));
    }
}

void GlaxnimateGraphicsView::keyReleaseEvent(QKeyEvent* event)
{
    if ( d->tool )
    {
        d->tool->key_release(d->key_event(event));
    }
}

void GlaxnimateGraphicsView::wheelEvent(QWheelEvent* event)
{
    if ( event->delta() < 0 )
        zoom_out();
    else
        zoom_in();
}

void GlaxnimateGraphicsView::zoom_in()
{
    zoom_view(1.25);
}

void GlaxnimateGraphicsView::zoom_out()
{
    zoom_view(0.8);
}

void GlaxnimateGraphicsView::translate_view(const QPointF& delta)
{
    translate(delta);
    d->expand_scene_rect(10);
}

void GlaxnimateGraphicsView::zoom_view(qreal factor)
{
    d->expand_scene_rect(std::max(width(), height()));
    zoom_view_anchor(factor, d->anchor_scene());
}

void GlaxnimateGraphicsView::zoom_view_anchor(qreal factor, const QPointF& scene_anchor)
{
    if ( d->zoom_factor*factor < 0.01 )
        return;

    d->expand_scene_rect(10);
    translate(scene_anchor);
    scale(factor, factor);
    translate(-scene_anchor);
    d->expand_scene_rect(0);

    d->zoom_factor *= factor;
    emit zoomed(d->zoom_factor);
}

void GlaxnimateGraphicsView::set_zoom(qreal factor)
{
    set_zoom_anchor(factor, d->anchor_scene());
}


void GlaxnimateGraphicsView::set_zoom_anchor(qreal factor, const QPointF& anchor)
{
    if ( factor < 0.01 )
        return;
    zoom_view_anchor(factor / d->zoom_factor, anchor);
}

qreal GlaxnimateGraphicsView::get_zoom_factor() const
{
    return d->zoom_factor;
}

void GlaxnimateGraphicsView::paintEvent(QPaintEvent *event)
{
    QGraphicsView::paintEvent(event);

    QPainter painter;
    painter.begin(viewport());
    painter.setRenderHints(renderHints());

    if ( d->mouse_view_mode == Private::Rotate || d->mouse_view_mode == Private::Scale )
    {
        QPoint p1 = d->move_last;
        QPoint p2 = d->transform_center;
        painter.setPen(QPen(QColor(150, 150, 150), 3));
        painter.drawLine(p1, p2);
        painter.setPen(QPen(QColor(50, 50, 50), 1));
        painter.drawLine(p1, p2);
    }
    else if ( d->tool )
    {
        d->tool->paint(d->paint_event(&painter));
    }
    painter.end();
}

void GlaxnimateGraphicsView::do_rotate(qreal radians, const QPointF& scene_anchor)
{
    translate(scene_anchor);
    rotate(qRadiansToDegrees(radians));
    translate(-scene_anchor);
    d->expand_scene_rect(10);
    d->rotation += radians;
    d->rotation = std::fmod(d->rotation, 2*M_PI);
    if ( d->rotation < 0 )
        d->rotation += 2*M_PI;
    emit rotated(d->rotation);
}

void GlaxnimateGraphicsView::set_rotation(qreal radians)
{
    do_rotate(radians-d->rotation, d->anchor_scene());
}

void GlaxnimateGraphicsView::view_fit(const QRect& fit_target)
{
    setTransform(QTransform());
    d->rotation = 0;
    d->zoom_factor = 1;
    emit rotated(0);

    if ( fit_target.isValid() && width() > 0 && height() > 0 )
    {
        d->expand_scene_rect(std::max(width(), height())*3);
        qreal factor = std::min(width() / qreal(fit_target.width()), height() / qreal(fit_target.height()));
        QPointF center(fit_target.center());
        zoom_view_anchor(factor, QPointF(0, 0));
        centerOn(center);
    }
    else
    {
        emit zoomed(1);
    }
}

void GlaxnimateGraphicsView::set_active_tool(tools::Tool* tool)
{
    d->tool = tool;
    if ( d->mouse_view_mode == Private::NoDrag )
        setCursor(tool->cursor());
}

void GlaxnimateGraphicsView::set_tool_target(GlaxnimateWindow* window)
{
    d->tool_target = window;
}
