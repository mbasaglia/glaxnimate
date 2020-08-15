#include "timeline_widget.hpp"

#include <QGraphicsObject>
#include <QStyleOptionGraphicsItem>
#include <QScrollBar>

#include "app/application.hpp"
#include "model/document.hpp"


class KeyframeItem : public QGraphicsObject
{
public:
    static const int icon_size = 16;
    static const int pen = 2;
    
    KeyframeItem(QGraphicsItem* parent) : QGraphicsObject(parent)
    {
        setFlags(
            QGraphicsItem::ItemIsSelectable|
            QGraphicsItem::ItemIsMovable|
            QGraphicsItem::ItemIgnoresTransformations
        );
    }
    
    QRectF boundingRect() const override
    {
        return QRectF(-icon_size/2-pen, -icon_size/2-pen, icon_size+2*pen, icon_size+2*pen);
    }
    
    void paint(QPainter * painter, const QStyleOptionGraphicsItem *, QWidget * widget) override
    {
        if ( isSelected() )
        {
            QColor sel_border = widget->palette().color(QPalette::Highlight);
            if ( parentItem()->isSelected() )
                sel_border = widget->palette().color(QPalette::HighlightedText);
            QColor sel_fill = sel_border;
            sel_fill.setAlpha(128);
            painter->setPen(QPen(sel_border, pen));
            painter->setBrush(sel_fill);
            painter->drawRect(boundingRect());
        }
        
        painter->drawPixmap(-icon_size, -icon_size/2, pix_enter);
        painter->drawPixmap(0, -icon_size/2, pix_exit);
    }
    
    
    void set_enter(model::KeyframeTransition::Descriptive enter)
    {
        icon_enter = icon_from_kdf(enter, "after");
        pix_enter = icon_enter.pixmap(icon_size);
    }
    
    void set_exit(model::KeyframeTransition::Descriptive exit)
    {
        icon_exit = icon_from_kdf(exit, "after");
        pix_exit = icon_enter.pixmap(icon_size);
    }
    
private:
    QIcon icon_from_kdf(model::KeyframeTransition::Descriptive desc, const char* ba)
    {
        QString icon_name = QString("images/keyframe/%1/%2.svg");
        QString which;
        switch ( desc )
        {
            case model::KeyframeTransition::Constant: which = "hold"; break;
            case model::KeyframeTransition::Linear: which = "linear"; break;
            case model::KeyframeTransition::Ease: which = "ease"; break;
            case model::KeyframeTransition::Custom: which = "custom"; break;
        }
        return QIcon(app::Application::instance()->data_file(icon_name.arg(ba).arg(which)));
    }
    
    QPixmap pix_enter;
    QPixmap pix_exit;
    QIcon icon_enter;
    QIcon icon_exit;
};

class AnimatableItem : public QGraphicsObject
{
public:
    AnimatableItem(model::AnimatableBase* animatable, int time_start, int time_end, int height)
        : animatable(animatable), time_start(time_start), time_end(time_end), height(height)
    {
        setFlags(QGraphicsItem::ItemIsSelectable);
        
        for ( int i = 0; i < animatable->keyframe_count(); i++ )
            add_keyframe(animatable, i);
    }
    
    void set_time_start(int time)
    {
        time_start = time;
        prepareGeometryChange();
    }
    
    void set_time_end(int time)
    {
        time_end = time;
        prepareGeometryChange();
    }
    
    void set_height(int h)
    {
        height = h;
        prepareGeometryChange();
    }
    
    QRectF boundingRect() const override
    {
        return QRectF(time_start, 0, time_end, height);
    }
    
    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) override
    {
        if ( isSelected() )
        {
            QColor selcol = widget->palette().color(QPalette::Highlight);
            painter->fillRect(option->rect, selcol);
        }
        
        QPen p(widget->palette().color(QPalette::Text), 1);
        p.setCosmetic(true);
        painter->setPen(p);
        painter->drawLine(option->rect.left(), height, option->rect.right(), height);
    }
    
    void add_keyframe(model::AnimatableBase* anim, int index)
    {
        model::KeyframeBase* kf = anim->keyframe(index);
        model::KeyframeBase* prev = index > 0 ? anim->keyframe(index-1) : nullptr;
        auto item = new KeyframeItem(this);
        item->setPos(kf->time(), 0);
        item->set_exit(kf->transition().before());
        item->set_enter(prev ? prev->transition().before() : model::KeyframeTransition::Constant);
        kf_items.push_back(item);
    }
    
    
private:
    model::AnimatableBase* animatable;
    std::vector<KeyframeItem*> kf_items;
    int time_start;
    int time_end;
    int height;
};

class TimelineWidget::Private
{
public:
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
    
    QRectF scene_rect()
    {
        return QRectF(start_time, -header_height, end_time, header_height+row_height*rows);
    }
    
    
    void add_animatable(model::AnimatableBase* anim)
    {
        AnimatableItem* item = new AnimatableItem(anim, start_time, end_time, row_height);
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
    }
    
    void paint_highligted_frame(TimelineWidget* parent, int frame, 
                                QPainter& painter, const QColor& color)
    {
        QPointF framep = parent->mapFromScene(QPoint(frame, 0));
        QPointF framep1 = parent->mapFromScene(QPoint(frame+1, 0));
        painter.fillRect(
            framep.x(),
            0,
            framep1.x() - framep.x() + 1,
            header_height,
            color
        );
    }
};

TimelineWidget::TimelineWidget(QWidget* parent)
    : QGraphicsView(parent), d(std::make_unique<Private>())
{
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

void TimelineWidget::add_container(model::AnimationContainer* cont)
{
    Q_UNUSED(cont)
}

void TimelineWidget::add_animatable(model::AnimatableBase* anim)
{
    d->add_animatable(anim);
}

void TimelineWidget::set_active(model::DocumentNode* node)
{
    clear();
    d->add_object(node);
    setSceneRect(d->scene_rect());
    reset_view();
}

void TimelineWidget::clear()
{
    d->scene.clear();
    d->rows = 0;
    d->anim_items.clear();
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
    }
    
    clear();
    
    if ( document )
    {
        connect(document->animation(), &model::AnimationContainer::first_frame_changed, this, &TimelineWidget::update_timeline_start);
        connect(document->animation(), &model::AnimationContainer::last_frame_changed, this, &TimelineWidget::update_timeline_end);
        update_timeline_end(document->animation()->last_frame.get());
        update_timeline_start(document->animation()->first_frame.get());
        connect(this, &TimelineWidget::frame_clicked, document, &model::Document::set_current_time);
        connect(document, &model::Document::current_time_changed, viewport(), (void (QWidget::*)())&QWidget::update);
    }
    
    d->document = document;
}

void TimelineWidget::update_timeline_end(model::FrameTime end)
{
    d->end_time = end;
    setSceneRect(d->scene_rect());
    d->adjust_min_scale(viewport()->width());
    for ( const auto& p : d->anim_items )
        p.second->set_time_end(end);
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
        if ( event->delta() < 0 )
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
    QGraphicsView::paintEvent(event);
    
    QPainter painter(viewport());
    
    painter.fillRect(event->rect().left(), 0, event->rect().right(), d->header_height,
                     palette().color(QPalette::Base));
    
    
    if ( d->document )
        d->paint_highligted_frame(this, d->document->current_time(), painter, 
                                  palette().color(QPalette::Text));
        
    if ( d->mouse_frame > -1 )
        d->paint_highligted_frame(this, d->mouse_frame, painter, palette().color(QPalette::Highlight));
    
    QPen dark(palette().color(QPalette::Text), 1);
    painter.setPen(dark);
    painter.drawLine(event->rect().left(), d->header_height, event->rect().right(), d->header_height);
    
    
    if ( event->rect().top() < d->header_height )
    {
        QColor frame_line = palette().color(QPalette::Text);
        frame_line.setAlphaF(0.5);
        QPen light(frame_line, 1);
        painter.setPen(light);
        QPointF scene_tl = mapToScene(event->rect().topLeft());
        QPointF scene_br = mapToScene(event->rect().bottomRight());
        int first_frame = qCeil(scene_tl.x());
        int last_frame = qFloor(scene_br.x());
        int small_height = d->header_height / 4;
        for ( int f = first_frame; f <= last_frame; f++ )
        {
            QPoint p1 = mapFromScene(f, scene_tl.y());
            int height = d->header_height;
            bool under_mouse = f == d->mouse_frame;
            bool current_frame = d->document && f == d->document->current_time();
            
            if ( f % d->frame_skip == 0 || under_mouse || current_frame )
            {
                bool draw = true;
                int fs = f / d->frame_skip * d->frame_skip;
                int box_x1 = mapFromScene(fs, 0).x();
                int box_x2 = mapFromScene(fs+d->frame_skip, 0).x();
                QRect text_rect(
                    QPoint(box_x1+1, small_height),
                    QPoint(box_x2, d->header_height-1)
                );
                
                if ( current_frame && !under_mouse && d->mouse_frame != -1 )
                {
                    int mfs = d->mouse_frame / d->frame_skip * d->frame_skip;
                    
                    if ( fs == mfs )
                        draw = false;
                }
                
                if ( draw )
                {
                    if ( under_mouse )
                    {
                        painter.fillRect(text_rect, palette().color(QPalette::Highlight));
                        painter.setPen(QPen(palette().color(QPalette::HighlightedText), 1));
                    }
                    else if ( current_frame )
                    {
                        painter.fillRect(text_rect, palette().color(QPalette::Text));
                        painter.setPen(QPen(palette().color(QPalette::Base), 1));
                    }
                    else 
                    {
                        painter.setPen(dark);
                    }
                
                    painter.drawText(text_rect, Qt::AlignLeft|Qt::AlignBottom,  QString::number(f));
                    painter.setPen(light);
                }
            }
            
            if ( f % d->frame_skip )
            {
                height = small_height;
            }
            
            painter.drawLine(QPoint(p1.x(), 0), QPoint(p1.x(), height));
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
    if ( event->y() > d->header_height )
        QGraphicsView::mousePressEvent(event);
}

void TimelineWidget::mouseMoveEvent(QMouseEvent* event)
{
    if ( event->y() > d->header_height )
        QGraphicsView::mouseMoveEvent(event);
    d->mouse_frame = mapToScene(event->pos()).x();
    viewport()->update();
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if ( event->y() > d->header_height )
        QGraphicsView::mouseReleaseEvent(event);
    else 
        emit frame_clicked(d->mouse_frame);
}

void TimelineWidget::leaveEvent(QEvent* event)
{
    QGraphicsView::leaveEvent(event);
    d->mouse_frame = -1;
}
