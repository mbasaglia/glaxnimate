#include "timeline_widget.hpp"

#include <QScrollBar>
#include <QWheelEvent>

#include "timeline_items.hpp"

class TimelineWidget::Private
{
public:
    TimelineWidget* parent;
    QGraphicsScene scene;
    int start_time = 0;
    int end_time = 0;
    int row_height = 24;
    int header_height = 24;
    int rows = 0;
    std::unordered_map<model::AnimatableBase*, AnimatableItem*> anim_items;
    qreal min_scale = 1;
    int frame_skip = 1;
    int min_gap = 32;
    int mouse_frame = -1;
    model::Document* document = nullptr;
    bool dragging_frame = false;
    int layer_start = 0;
    int layer_end = 0;
    model::AnimationContainer* limit = nullptr;

    int rounded_end_time()
    {
        return time_round_to_ticks(end_time);
    }

    int time_round_to_ticks(int time)
    {
        return (time/frame_skip + 1) * frame_skip;
    }

    QRectF scene_rect()
    {
        return QRectF(
            QPointF(start_time, -header_height),
            QPointF(rounded_end_time(), std::max(row_height*(rows+1), parent->height()))
        );
    }


    void add_animatable(model::AnimatableBase* anim)
    {
        AnimatableItem* item = new AnimatableItem(anim, start_time, rounded_end_time(), row_height);
        connect(item, &AnimatableItem::animatable_clicked, parent, &TimelineWidget::animatable_clicked);
        item->setPos(0, rows * row_height);
        anim_items[anim] = item;
        scene.addItem(item);
        rows += 1;
    }

    void add_object(model::Object* obj)
    {
        for ( auto prop : obj->properties() )
        {
            auto flags = prop->traits().flags;
            if ( flags & model::PropertyTraits::Animated )
                add_animatable(static_cast<model::AnimatableBase*>(prop));
            else if ( prop->traits().type == model::PropertyTraits::Object && !(flags & model::PropertyTraits::List) )
                add_object(static_cast<model::SubObjectPropertyBase*>(prop)->sub_object());
        }
    }

    void adjust_min_scale(int wpw)
    {
        if ( min_scale == 0 || scene_rect().width() == 0 )
            min_scale = 1;
        else
            min_scale = wpw / scene_rect().width();
    }

    void update_frame_skip(const QTransform& tr)
    {
        frame_skip = qCeil(min_gap / tr.m11());
        update_end_time();
    }

    void update_end_time()
    {
        auto et = rounded_end_time();
        for ( const auto& p : anim_items )
            p.second->set_time_end(et);
    }

    void paint_highligted_frame(int frame, QPainter& painter, const QBrush& color)
    {
        QPointF framep = parent->mapFromScene(QPoint(frame, 0));
        QPointF framep1 = parent->mapFromScene(QPoint(frame+1, 0));
        painter.fillRect(
            framep.x() + 1,
            0,
            framep1.x() - framep.x() - 0.5,
            header_height - 0.5,
            color
        );
    }

    void clear()
    {
        scene.clear();
        rows = 0;
        anim_items.clear();
    }

    model::AnimationContainer* anim(model::DocumentNode* node)
    {
        while ( node )
        {
            const QMetaObject* mo = node->metaObject();
            if ( mo->inherits(&model::Layer::staticMetaObject) )
                return static_cast<model::Layer*>(node)->animation.get();
            else if ( mo->inherits(&model::Composition::staticMetaObject) )
                return static_cast<model::Composition*>(node)->animation.get();

            node = node->docnode_parent();
        }

        return document->main()->animation.get();
    }

    QRectF frame_text_rect(int f, TimelineWidget* parent)
    {

        int fs = f / frame_skip * frame_skip;
        int box_x1 = parent->mapFromScene(fs, 0).x();
        int box_x2 = parent->mapFromScene(fs+frame_skip, 0).x();
        return QRectF(
            QPointF(box_x1+1, header_height / 4),
            QPointF(box_x2-0.5, header_height-0.5)
        );
    }

    void paint_frame_rect(TimelineWidget* parent, QPainter& painter, int f, const QBrush& brush, const QPen& pen)
    {
        QRectF text_rect = frame_text_rect(f, parent);
        painter.fillRect(text_rect, brush);
        painter.setPen(pen);
        painter.drawText(text_rect, Qt::AlignLeft|Qt::AlignBottom,  QString::number(f));
    }
};

TimelineWidget::TimelineWidget(QWidget* parent)
    : QGraphicsView(parent), d(std::make_unique<Private>())
{
    d->parent = this;
    setMouseTracking(true);
    setInteractive(true);
    setRenderHint(QPainter::Antialiasing);
    setScene(&d->scene);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setTransformationAnchor(AnchorUnderMouse);
}

TimelineWidget::~TimelineWidget()
{
}

void TimelineWidget::add_animatable(model::AnimatableBase* anim)
{
    d->add_animatable(anim);
}

void TimelineWidget::set_active(model::DocumentNode* node)
{
    clear();
    if ( node )
    {
        d->add_object(node);
        setSceneRect(d->scene_rect());
        set_anim_container(d->anim(node));
        reset_view();
    }
}

void TimelineWidget::clear()
{
    d->clear();
}

int TimelineWidget::row_height() const
{
    return d->row_height;
}

void TimelineWidget::set_row_height(int w)
{
    d->row_height = w;
}

void TimelineWidget::set_document(model::Document* document)
{
    if ( d->document )
    {
        disconnect(this, nullptr, d->document, nullptr);
        disconnect(d->document->main()->animation.get(), nullptr, this, nullptr);
        disconnect(d->document, nullptr, viewport(), nullptr);
    }

    clear();

    if ( document )
    {
        connect(document->main()->animation.get(), &model::AnimationContainer::first_frame_changed, this, &TimelineWidget::update_timeline_start);
        connect(document->main()->animation.get(), &model::AnimationContainer::last_frame_changed, this, &TimelineWidget::update_timeline_end);
        update_timeline_end(document->main()->animation->last_frame.get());
        update_timeline_start(document->main()->animation->first_frame.get());
        connect(this, &TimelineWidget::frame_clicked, document, &model::Document::set_current_time);
        connect(document, &model::Document::current_time_changed, viewport(), (void (QWidget::*)())&QWidget::update);
        set_anim_container(document->main()->animation.get());
    }
    else
    {
        set_anim_container(nullptr);
    }

    d->document = document;
}

void TimelineWidget::update_timeline_end(model::FrameTime end)
{
    d->end_time = end;
    setSceneRect(d->scene_rect());
    d->adjust_min_scale(viewport()->width());
    d->update_end_time();
}

void TimelineWidget::update_timeline_start(model::FrameTime start)
{
    d->start_time = start;
    setSceneRect(d->scene_rect());
    d->adjust_min_scale(viewport()->width());
    for ( const auto& p : d->anim_items )
        p.second->set_time_start(start);
}

void TimelineWidget::wheelEvent(QWheelEvent* event)
{
    if ( event->modifiers() & Qt::ControlModifier )
    {
        if ( event->angleDelta().y() < 0 )
        {
            qreal scale_by = 0.8;
            qreal cs = transform().m11();
            if ( cs * scale_by < d->min_scale )
                scale_by = d->min_scale / cs;
            scale(scale_by, 1);
        }
        else
        {
            scale(1.25, 1);
        }

        d->update_frame_skip(transform());
    }
    else
    {
        if ( event->modifiers() & Qt::ShiftModifier )
            QApplication::sendEvent(verticalScrollBar(), event);
        else
            QApplication::sendEvent(horizontalScrollBar(), event);
    }
}

void TimelineWidget::paintEvent(QPaintEvent* event)
{
    QPen dark(palette().text(), 1);
    int small_height = d->header_height / 4;
    QPointF scene_tl = mapToScene(event->rect().topLeft());
    QPointF scene_br = mapToScene(event->rect().bottomRight());
    QBrush disabled = palette().brush(QPalette::Disabled, QPalette::Base);

    // bg
    QPainter painter;
    painter.begin(viewport());
    // gray out the area after the outside the layer range
    if ( d->layer_start > d->start_time )
    {
        painter.fillRect(
            QRectF(mapFromScene(d->start_time, scene_tl.y()), mapFromScene(d->layer_start, scene_br.y())),
            disabled
        );
    }
    painter.fillRect(
        QRectF(mapFromScene(d->layer_end, scene_tl.y()), mapFromScene(d->rounded_end_time(), scene_br.y())),
        disabled
    );
    painter.end();

    // scene
    QGraphicsView::paintEvent(event);

    // fg
    painter.begin(viewport());


    // Vertical line for the current keyframe
    if ( d->document )
    {
        painter.setPen(dark);
        int cur_x = mapFromScene(d->document->current_time(), 0).x();
        painter.drawLine(cur_x, event->rect().top(), cur_x, event->rect().bottom());
    }

    // Fill the header background
    painter.fillRect(event->rect().left(), 0, event->rect().right(), d->header_height, disabled);

    // Gray out ticks outside layer range
    painter.fillRect(
        QRectF(
            QPointF(mapFromScene(d->layer_start, 0).x(), 0),
            QPointF(mapFromScene(d->layer_end+1, 0).x(), small_height)
        ),
        palette().base()
    );
    painter.fillRect(
        QRectF(
            QPointF(mapFromScene(d->time_round_to_ticks(d->layer_start) - d->frame_skip, 0).x(), small_height),
            QPointF(mapFromScene(d->time_round_to_ticks(d->layer_end), 0).x(), d->header_height)
        ),
        palette().base()
    );

    painter.setPen(dark);
    painter.drawLine(event->rect().left(), d->header_height, event->rect().right(), d->header_height);


    // Draw header ticks marks
    if ( event->rect().top() < d->header_height )
    {
        QColor frame_line = palette().color(QPalette::Text);
        frame_line.setAlphaF(0.5);
        QPen light(frame_line, 1);
        painter.setPen(light);
        int first_frame = qCeil(scene_tl.x());
        int last_frame = qFloor(scene_br.x());
        for ( int f = first_frame; f <= last_frame; f++ )
        {
            QPoint p1 = mapFromScene(f, scene_tl.y());
            int height = d->header_height;

            if ( f % d->frame_skip == 0 )
            {
                d->paint_frame_rect(this, painter, f, Qt::NoBrush, dark);
                painter.setPen(light);
            }
            else
            {
                height = small_height;
            }

            painter.drawLine(QPoint(p1.x(), 0), QPoint(p1.x(), height));
        }

        d->paint_frame_rect(this, painter, d->layer_start, palette().base(), dark);

        if ( d->document )
        {
            d->paint_highligted_frame(d->document->current_time(), painter, palette().text());
            d->paint_frame_rect(this, painter, d->document->current_time(), palette().text(), QPen(palette().base(), 1));
        }

        if ( d->mouse_frame > -1 )
        {
            d->paint_highligted_frame(d->mouse_frame, painter, palette().highlight());
            d->paint_frame_rect(this, painter, d->mouse_frame, palette().highlight(), QPen(palette().highlightedText(), 1));
        }
    }
}

void TimelineWidget::reset_view()
{
    setTransform(QTransform::fromScale(d->min_scale, 1));
    d->update_frame_skip(transform());
}

void TimelineWidget::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    setSceneRect(d->scene_rect());
    d->adjust_min_scale(viewport()->width());
    if ( transform().m11() < d->min_scale )
        reset_view();
}

void TimelineWidget::scrollContentsBy(int dx, int dy)
{
    QGraphicsView::scrollContentsBy(dx, dy);
    viewport()->update();
}

void TimelineWidget::mousePressEvent(QMouseEvent* event)
{
    d->mouse_frame = qRound(mapToScene(event->pos()).x());

    if ( event->y() > d->header_height )
    {
        QGraphicsView::mousePressEvent(event);
    }
    else if ( event->button() == Qt::LeftButton )
    {
        d->dragging_frame = true;
        emit frame_clicked(d->mouse_frame);
    }
}

void TimelineWidget::mouseMoveEvent(QMouseEvent* event)
{
    d->mouse_frame = qRound(mapToScene(event->pos()).x());

    if ( d->dragging_frame && (event->buttons() & Qt::LeftButton) )
    {
        emit frame_clicked(d->mouse_frame);
    }
    else if ( event->y() > d->header_height )
    {
        QGraphicsView::mouseMoveEvent(event);
    }

    viewport()->update();
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if ( event->y() <= d->header_height || d->dragging_frame )
    {
        d->dragging_frame = false;
        emit frame_clicked(d->mouse_frame);
    }
    else
    {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

void TimelineWidget::leaveEvent(QEvent* event)
{
    QGraphicsView::leaveEvent(event);
    d->mouse_frame = -1;
}

int TimelineWidget::header_height() const
{
    return d->header_height;
}

void TimelineWidget::select(model::AnimatableBase* anim)
{
    d->scene.clearSelection();
    if ( anim )
    {
        auto it = d->anim_items.find(anim);
        if ( it != d->anim_items.end() )
            it->second->setSelected(true);
    }
}

model::AnimatableBase * TimelineWidget::animatable_at(const QPoint& viewport_pos)
{
    for ( QGraphicsItem* it : items(viewport_pos) )
        if ( auto anit = dynamic_cast<AnimatableItem*>(it) )
            return anit->animatable;
    return nullptr;
}

std::pair<model::KeyframeBase*, model::KeyframeBase*> TimelineWidget::keyframe_at(const QPoint& viewport_pos)
{
    for ( QGraphicsItem* it : items(viewport_pos) )
    {
        if ( auto kfit = dynamic_cast<KeyframeSplitItem*>(it) )
        {
            auto anit = static_cast<AnimatableItem*>(it->parentItem());
            return anit->keyframes(kfit);
        }
    }
    return {nullptr, nullptr};
}

void TimelineWidget::keyPressEvent(QKeyEvent* event)
{
    if ( !d->document )
        return;

    /// \todo figure why pageup/end etc aren't received here...
    switch ( event->key() )
    {
        case Qt::Key_PageDown:
        case Qt::Key_Left:
            if ( d->document->current_time() - 1 >= d->start_time )
                emit frame_clicked(d->document->current_time() - 1);
            event->accept();
            break;
        case Qt::Key_PageUp:
        case Qt::Key_Right:
            if ( d->document->current_time() + 1 <= d->end_time )
                emit frame_clicked(d->document->current_time() + 1);
            event->accept();
            break;
        case Qt::Key_Home:
            emit frame_clicked(d->start_time);
            event->accept();
            break;
        case Qt::Key_End:
            emit frame_clicked(d->end_time);
            event->accept();
            break;
    }

    QGraphicsView::keyPressEvent(event);
}

void TimelineWidget::set_anim_container(model::AnimationContainer* cont)
{

    if ( d->limit )
    {
        disconnect(d->limit, nullptr, this, nullptr);
    }

    d->limit = cont;

    if ( d->limit )
    {
        connect(cont, &model::AnimationContainer::first_frame_changed, this, &TimelineWidget::update_layer_start);
        connect(cont, &model::AnimationContainer::last_frame_changed, this, &TimelineWidget::update_layer_end);
        update_layer_end(cont->last_frame.get());
        update_layer_start(cont->first_frame.get());
    }
}

void TimelineWidget::update_layer_end(model::FrameTime end)
{
    d->layer_end = end;
    viewport()->update();
}

void TimelineWidget::update_layer_start(model::FrameTime start)
{
    d->layer_start = start;
    viewport()->update();
}
