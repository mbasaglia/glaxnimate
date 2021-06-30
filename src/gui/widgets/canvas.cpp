#include "canvas.hpp"

#include <cmath>

#include <QMouseEvent>
#include <QtMath>
#include <QTouchEvent>

#include "command/undo_macro_guard.hpp"
#include "tools/base.hpp"
#include "graphics/document_scene.hpp"



class Canvas::Private
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

    Private(Canvas* view) : view(view) {}

    Canvas* view;

    qreal zoom_factor = 1;
    qreal rotation = 0;
    tools::Tool* tool = nullptr;
    glaxnimate::gui::DocumentEnvironment* tool_target = nullptr;
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
    bool resize_fit = true;
    QPainterPath clip;
    QPainterPath in_clip;
    qreal pinch_zoom = 1;

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
    tools::Event event()
    {
        return {
            view,
            scene(),
            tool_target
        };
    }

    tools::MouseEvent mouse_event(QMouseEvent* ev)
    {
        return {
            event(),
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
            event(),
            painter,
            view->palette()
        };
    }

    tools::KeyEvent key_event(QKeyEvent* ev)
    {
        return {
            event(),
            ev
        };
    }

    graphics::DocumentScene* scene()
    {
        return static_cast<graphics::DocumentScene*>(view->scene());
    }

    model::Document* document()
    {
        return scene()->document();
    }
};


Canvas::Canvas(QWidget* parent)
    : QGraphicsView(parent), d(std::make_unique<Private>(this))
{
    setMouseTracking(true);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(NoAnchor);
    setResizeAnchor(NoAnchor);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameStyle(QFrame::NoFrame);
}

Canvas::~Canvas() = default;

void Canvas::mousePressEvent(QMouseEvent* event)
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

void Canvas::mouseMoveEvent(QMouseEvent* event)
{
//     QGraphicsView::mouseMoveEvent(event);

    QPoint mpos = event->pos();
    QPointF scene_pos = mapToScene(mpos);
    emit mouse_moved(scene_pos);

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



void Canvas::mouseReleaseEvent(QMouseEvent * event)
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

    d->press_button = Qt::NoButton;
    d->update_mouse_cursor();
}

void Canvas::mouseDoubleClickEvent(QMouseEvent* event)
{
    if ( d->tool )
    {
        d->tool->mouse_double_click(d->mouse_event(event));
    }
}

void Canvas::keyPressEvent(QKeyEvent* event)
{
    if ( d->tool )
    {
        d->tool->key_press(d->key_event(event));
    }
}

void Canvas::keyReleaseEvent(QKeyEvent* event)
{
    if ( d->tool )
    {
        d->tool->key_release(d->key_event(event));
    }
}

void Canvas::wheelEvent(QWheelEvent* event)
{
    if ( event->angleDelta().y() < 0 )
        zoom_out();
    else
        zoom_in();
}

void Canvas::zoom_in()
{
    zoom_view(1.25);
}

void Canvas::zoom_out()
{
    zoom_view(0.8);
}

void Canvas::translate_view(const QPointF& delta)
{
    translate(delta);
    d->expand_scene_rect(10);
}

void Canvas::zoom_view(qreal factor)
{
    d->expand_scene_rect(std::max(width(), height()));
    zoom_view_anchor(factor, d->anchor_scene());
}

void Canvas::zoom_view_anchor(qreal factor, const QPointF& scene_anchor)
{
    if ( d->zoom_factor*factor < 0.01 )
        return;

    d->expand_scene_rect(10);
    translate(scene_anchor);
    scale(factor, factor);
    translate(-scene_anchor);
    d->expand_scene_rect(0);

    d->zoom_factor *= factor;

    d->resize_fit = false;
    emit zoomed(d->zoom_factor);
}

void Canvas::set_zoom(qreal factor)
{
    set_zoom_anchor(factor, d->anchor_scene());
}

void Canvas::set_zoom_anchor(qreal factor, const QPointF& anchor)
{
    if ( factor < 0.01 )
        return;
    zoom_view_anchor(factor / d->zoom_factor, anchor);
}

qreal Canvas::get_zoom_factor() const
{
    return d->zoom_factor;
}

void Canvas::flip_horizontal()
{
    auto anchor = mapToScene(width()/2, height()/2);
    auto angle = d->rotation * 180 / M_PI;
    translate(anchor);
    rotate(-angle);
    scale(-1, 1);
    rotate(angle);
    translate(-anchor);
}

void Canvas::paintEvent(QPaintEvent *event)
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

    const QPalette& palette = this->palette();
    painter.setPen(Qt::NoPen);
    painter.setBrush(palette.window());
    painter.drawPath(d->clip);

    painter.setBrush(Qt::NoBrush);

    QPen pen(palette.mid(), 1);

    if ( d->document() && d->document()->record_to_keyframe() )
    {
        if ( hasFocus() )
            pen.setColor(QColor(255, 0, 0));
        else
            pen.setColor(math::lerp(pen.color(), QColor(255, 0, 0), 0.25));
    }
    else
    {
        if ( hasFocus() )
            pen.setBrush(palette.highlight());
    }

    painter.setPen(pen);
    painter.drawPath(d->in_clip);

    painter.end();

}

void Canvas::do_rotate(qreal radians, const QPointF& scene_anchor)
{
    translate(scene_anchor);
    rotate(qRadiansToDegrees(radians));
    translate(-scene_anchor);
    d->expand_scene_rect(10);
    d->rotation += radians;
    d->rotation = std::fmod(d->rotation, 2*M_PI);
    if ( d->rotation < 0 )
        d->rotation += 2*M_PI;


    d->resize_fit = false;
    emit rotated(d->rotation);
}

void Canvas::set_rotation(qreal radians)
{
    do_rotate(radians-d->rotation, d->anchor_scene());
}

void Canvas::view_fit()
{
    setTransform(QTransform());
    d->rotation = 0;
    d->zoom_factor = 1;
    emit rotated(0);


    QRect fit_target;

    if ( d->tool_target->document() )
        fit_target = QRect(
            -32,
            -32,
            d->tool_target->document()->main()->width.get() + 64,
            d->tool_target->document()->main()->height.get() + 64
        );

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

    d->resize_fit = true;
}

void Canvas::set_active_tool(tools::Tool* tool)
{
    if ( d->tool )
    {
        d->tool->disable_event(d->event());
        disconnect(d->tool, nullptr, this, nullptr);
    }

    d->tool = tool;

    d->tool->enable_event(d->event());

    connect(d->tool, &tools::Tool::cursor_changed, this, &QWidget::setCursor);

    if ( d->mouse_view_mode == Private::NoDrag )
        setCursor(tool->cursor());
}

void Canvas::set_tool_target(glaxnimate::gui::DocumentEnvironment* window)
{
    d->tool_target = window;
}

void Canvas::translate(const QPointF& p)
{
     d->resize_fit = false;
     QGraphicsView::translate(p.x(), p.y());
}

void Canvas::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    if ( d->resize_fit )
        view_fit();

    d->clip = {};
    d->clip.setFillRule(Qt::OddEvenFill);
    d->clip.addRect(QRectF(viewport()->rect()));
    QPainterPath pp;
    pp.addRoundedRect(QRectF(viewport()->rect()), 4, 4);
    d->in_clip = pp.toReversed();
    d->clip.addPath(d->in_clip);
}

void Canvas::changeEvent(QEvent* event)
{
    QWidget::changeEvent ( event );

    if ( event->type() == QEvent::PaletteChange ) {
        scene()->setPalette(palette());
    }
}

void Canvas::dragEnterEvent(QDragEnterEvent* event)
{
    if ( event->mimeData()->hasFormat("application/x.glaxnimate-asset-uuid") ) {
        event->setDropAction(Qt::LinkAction);
        event->acceptProposedAction();
    }
}

void Canvas::dragMoveEvent(QDragMoveEvent* event)
{
    event->acceptProposedAction();
}


void Canvas::dropEvent(QDropEvent* event)
{
    event->acceptProposedAction();
    emit dropped(event->mimeData());
}

bool Canvas::event(QEvent* event)
{
    if ( event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut )
        viewport()->update();

    return QGraphicsView::event(event);
}

bool Canvas::viewportEvent(QEvent *event)
{
    switch ( event->type() )
    {
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
        {
            QTouchEvent *touch_event = static_cast<QTouchEvent *>(event);

            QList<QTouchEvent::TouchPoint> touch_points = touch_event->touchPoints();

            if (touch_points.count() == 2)
            {
                const QTouchEvent::TouchPoint &p0 = touch_points.first();
                const QTouchEvent::TouchPoint &p1 = touch_points.last();

                qreal initial_distance = math::length(p0.startPos() - p1.startPos());

                if ( initial_distance > 0 )
                {
                    qreal distance = math::length(p0.pos() - p1.pos());
                    QPointF travel = (p0.pos() - p0.startPos() + p1.pos() - p1.startPos()) / 2;
                    qreal travel_distance = math::length(travel);

                    // pinch
                    if ( math::abs(distance - initial_distance) > travel_distance )
                    {
                        QPointF center = (p0.startScenePos() + p1.startScenePos()) / 2;
                        qreal scale_by = distance / initial_distance;

                        if ( touch_event->touchPointStates() & Qt::TouchPointReleased )
                        {
                            d->pinch_zoom *= scale_by;
                            scale_by = 1;
                        }

                        set_zoom_anchor(d->pinch_zoom * scale_by, center);
                    }
                    // pan
                    else
                    {
                        QPointF scene_travel = (p0.scenePos() - p0.lastScenePos() + p1.scenePos() - p1.lastScenePos()) / 2;
                        translate_view(scene_travel);
                    }
                }

                return true;
            }
        }
        default:
            break;
    }

    return QGraphicsView::viewportEvent(event);
}
