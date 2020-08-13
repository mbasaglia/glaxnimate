#include "timeline_widget.hpp"

#include <QGraphicsObject>

#include "app/application.hpp"
#include "model/document.hpp"


class KeyframeItem : public QGraphicsObject
{
public:
    static const int icon_size = 16;
    static const int pen = 2;
    
    KeyframeItem()
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

class TimelineWidget::Private
{
public:
    QGraphicsScene scene;
    int start_time = 0;
    int end_time = 0;
    int row_height = 24;
    int header_height = 24;
    int rows = 0;
    std::unordered_map<model::AnimatableBase*, KeyframeItem*> kf_items;
    
    QRectF scene_rect()
    {
        return QRectF(start_time, -header_height, end_time, header_height+row_height*rows);
    }
    
    void add_keyframe(model::AnimatableBase* anim, int index)
    {
        model::KeyframeBase* kf = anim->keyframe(index);
        model::KeyframeBase* prev = index > 0 ? anim->keyframe(index-1) : nullptr;
        auto item = new KeyframeItem();
        item->setPos(kf->time(), rows * row_height);
        item->set_exit(kf->transition().before());
        item->set_enter(prev ? prev->transition().before() : model::KeyframeTransition::Constant);
    }
};

TimelineWidget::TimelineWidget(QWidget* parent)
    : QGraphicsView(parent), d(std::make_unique<Private>())
{
    setInteractive(true);
    setRenderHint(QPainter::Antialiasing);
    setScene(&d->scene);
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
    for ( int i = 0; i < anim->keyframe_count(); i++ )
        d->add_keyframe(anim, i);
}

void TimelineWidget::clear()
{
    d->scene.clear();
    d->rows = 0;
    d->kf_items.clear();
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
    clear();
    connect(document->animation(), &model::AnimationContainer::first_frame_changed, this, &TimelineWidget::update_timeline_start);
    connect(document->animation(), &model::AnimationContainer::last_frame_changed, this, &TimelineWidget::update_timeline_end);
    update_timeline_end(document->animation()->last_frame.get());
    update_timeline_start(document->animation()->first_frame.get());
}

void TimelineWidget::update_timeline_end(model::FrameTime end)
{
    d->end_time = end;
    setSceneRect(d->scene_rect());
}

void TimelineWidget::update_timeline_start(model::FrameTime start)
{
    d->start_time = start;
    setSceneRect(d->scene_rect());
}

void TimelineWidget::wheelEvent(QWheelEvent* event)
{
    if ( event->modifiers() & Qt::ControlModifier )
    {
        if ( event->delta() < 0 )
            scale(0.8, 1);
        else
            scale(1.25, 1);
    }
    else
    {
        QGraphicsView::wheelEvent(event);
        viewport()->update();
    }
}

void TimelineWidget::paintEvent(QPaintEvent* event)
{
    QGraphicsView::paintEvent(event);
    
    QColor text = palette().color(QPalette::Text);
    QPen pen(text, 1);
    QPainter painter(viewport());
    painter.setPen(pen);
    painter.setBrush(Qt::transparent);
    painter.drawLine(0, d->header_height, viewport()->width(), d->header_height);
}

