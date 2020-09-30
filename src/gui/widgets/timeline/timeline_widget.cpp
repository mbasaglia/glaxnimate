#include "timeline_widget.hpp"

#include <QGraphicsObject>
#include <QStyleOptionGraphicsItem>
#include <QScrollBar>
#include <QGraphicsSceneMouseEvent>
#include <QWheelEvent>

#include "app/application.hpp"
#include "model/document.hpp"
#include "command/animation_commands.hpp"

class KeyframeSplitItem : public QGraphicsObject
{
    Q_OBJECT

public:
    static const int icon_size = 16;
    static const int pen = 2;

    KeyframeSplitItem(QGraphicsItem* parent) : QGraphicsObject(parent)
    {
        setFlags(
            QGraphicsItem::ItemIsSelectable|
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

        painter->drawPixmap(-icon_size/2, -icon_size/2, pix_enter);
        painter->drawPixmap(0, -icon_size/2, pix_exit);
    }


    void set_enter(model::KeyframeTransition::Descriptive enter)
    {
        icon_enter = icon_from_kdf(enter, "finish");
        pix_enter = icon_enter.pixmap(icon_size);
        update();
    }

    void set_exit(model::KeyframeTransition::Descriptive exit)
    {
        icon_exit = icon_from_kdf(exit, "start");
        pix_exit = icon_exit.pixmap(icon_size);
        update();
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent * event) override
    {
        if ( event->button() == Qt::LeftButton )
        {
            event->accept();
            bool multi_select = (event->modifiers() & (Qt::ControlModifier|Qt::ShiftModifier)) != 0;
            if ( !multi_select && !isSelected() )
                scene()->clearSelection();

            setSelected(true);
            for ( auto item : scene()->selectedItems() )
            {
                if ( auto kf = dynamic_cast<KeyframeSplitItem*>(item) )
                    kf->drag_init();
            }
        }
        else
        {
            QGraphicsObject::mousePressEvent(event);
        }
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent * event) override
    {
        if ( (event->buttons() & Qt::LeftButton) && isSelected() )
        {
            event->accept();
            qreal delta = qRound(event->scenePos().x()) - drag_start;
            for ( auto item : scene()->selectedItems() )
            {
                if ( auto kf = dynamic_cast<KeyframeSplitItem*>(item) )
                    kf->drag_move(delta);
            }
        }
        else
        {
            QGraphicsObject::mouseMoveEvent(event);
        }
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent * event) override
    {
        if ( event->button() == Qt::LeftButton && isSelected() )
        {
            event->accept();
            for ( auto item : scene()->selectedItems() )
            {
                if ( auto kf = dynamic_cast<KeyframeSplitItem*>(item) )
                    kf->drag_end();
            }
        }
        else
        {
            QGraphicsObject::mouseReleaseEvent(event);
        }
    }

signals:
    void dragged(model::FrameTime t);

private:
    QIcon icon_from_kdf(model::KeyframeTransition::Descriptive desc, const char* ba)
    {
        QString icon_name = QString("images/keyframe/%1/%2.svg");
        QString which;
        switch ( desc )
        {
            case model::KeyframeTransition::Hold: which = "hold"; break;
            case model::KeyframeTransition::Linear: which = "linear"; break;
            case model::KeyframeTransition::Ease: which = "ease"; break;
            case model::KeyframeTransition::Custom: which = "custom"; break;
        }
        return QIcon(app::Application::instance()->data_file(icon_name.arg(ba).arg(which)));
    }

    void drag_init()
    {
        drag_start = x();
    }

    void drag_move(qreal delta)
    {
        setX(drag_start+delta);
    }

    void drag_end()
    {
        emit dragged(x());
    }

    QPixmap pix_enter;
    QPixmap pix_exit;
    QIcon icon_enter;
    QIcon icon_exit;
    model::FrameTime drag_start;
};

class AnimatableItem : public QGraphicsObject
{
    Q_OBJECT

public:
    AnimatableItem(model::AnimatableBase* animatable, int time_start, int time_end, int height)
        : animatable(animatable), time_start(time_start), time_end(time_end), height(height)
    {
        setFlags(QGraphicsItem::ItemIsSelectable);

        for ( int i = 0; i < animatable->keyframe_count(); i++ )
            add_keyframe(i);


        connect(animatable, &model::AnimatableBase::keyframe_added, this, &AnimatableItem::add_keyframe);
        connect(animatable, &model::AnimatableBase::keyframe_removed, this, &AnimatableItem::remove_keyframe);
        connect(animatable, &model::AnimatableBase::keyframe_updated, this, &AnimatableItem::update_keyframe);
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

    std::pair<model::KeyframeBase*, model::KeyframeBase*> keyframes(KeyframeSplitItem* item)
    {
        for ( int i = 0; i < int(kf_split_items.size()); i++ )
        {
            if ( kf_split_items[i] == item )
            {
                if ( i == 0 )
                    return {nullptr, animatable->keyframe(i)};
                return {animatable->keyframe(i-1), animatable->keyframe(i)};
            }
        }

        return {nullptr, nullptr};
    }

    void mousePressEvent(QGraphicsSceneMouseEvent * event) override
    {
        bool sel = isSelected();
        QGraphicsObject::mousePressEvent(event);
        if ( !sel && isSelected() )
            emit animatable_clicked(animatable);
    }

public slots:
    void add_keyframe(int index)
    {
        model::KeyframeBase* kf = animatable->keyframe(index);
        if ( index == 0 && !kf_split_items.empty() )
            kf_split_items[0]->set_enter(kf->transition().after());

        model::KeyframeBase* prev = index > 0 ? animatable->keyframe(index-1) : nullptr;
        auto item = new KeyframeSplitItem(this);
        item->setPos(kf->time(), height / 2.0);
        item->set_exit(kf->transition().before());
        item->set_enter(prev ? prev->transition().after() : model::KeyframeTransition::Hold);
        kf_split_items.insert(kf_split_items.begin() + index, item);

        connect(&kf->transition(), &model::KeyframeTransition::after_changed, this, &AnimatableItem::transition_changed_after);
        connect(&kf->transition(), &model::KeyframeTransition::before_changed, this, &AnimatableItem::transition_changed_before);
        connect(item, &KeyframeSplitItem::dragged, this, &AnimatableItem::keyframe_dragged);
    }

    void remove_keyframe(int index)
    {
        delete kf_split_items[index];
        kf_split_items.erase(kf_split_items.begin() + index);
        if ( index < int(kf_split_items.size()) && index > 0 )
        {
            kf_split_items[index]->set_enter(animatable->keyframe(index-1)->transition().after());
        }
    }

signals:
    void animatable_clicked(model::AnimatableBase* animatable);

private slots:
    void transition_changed_before(model::KeyframeTransition::Descriptive d)
    {
        int index = transition_changed_index();
        if ( index == -1 )
            return;

        kf_split_items[index]->set_exit(d);
    }

    void transition_changed_after(model::KeyframeTransition::Descriptive d)
    {
        int index = transition_changed_index();
        if ( index == -1 )
            return;

        index += 1;
        if ( index >= int(kf_split_items.size()) )
            return;

        kf_split_items[index]->set_enter(d);
    }

    void keyframe_dragged(model::FrameTime t)
    {
        auto it = std::find(kf_split_items.begin(), kf_split_items.end(), static_cast<KeyframeSplitItem*>(sender()));
        if ( it != kf_split_items.end() )
        {
            int index = it - kf_split_items.begin();
            if ( animatable->keyframe(index)->time() != t )
            {
                animatable->object()->document()->undo_stack().push(
                    new command::MoveKeyframe(animatable, index, t)
                );
            }
        }
    }

    void update_keyframe(int index, model::KeyframeBase* kf)
    {
        auto item_start = kf_split_items[index];
        item_start->setPos(kf->time(), height / 2.0);
        item_start->set_exit(kf->transition().before());

        if ( index < int(kf_split_items.size()) - 1 )
        {
            auto item_end = kf_split_items[index];
            item_end->set_enter(kf->transition().after());
        }
    }

private:
    int transition_changed_index()
    {
        model::KeyframeTransition* s = static_cast<model::KeyframeTransition*>(sender());

        for ( int i = 0, e = animatable->keyframe_count(); i < e; i++ )
        {
            if ( &animatable->keyframe(i)->transition() == s )
                return i;
        }

        return -1;
    }

public:
    model::AnimatableBase* animatable;
    std::vector<KeyframeSplitItem*> kf_split_items;
    int time_start;
    int time_end;
    int height;
};

#include "timeline_widget.moc"

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

    QRectF scene_rect()
    {
        return QRectF(
            QPointF(start_time, -header_height),
            QPointF(end_time, std::max(row_height*(rows+1), parent->height()))
        );
    }


    void add_animatable(model::AnimatableBase* anim)
    {
        AnimatableItem* item = new AnimatableItem(anim, start_time, end_time, row_height);
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
    }

    void paint_highligted_frame(int frame, QPainter& painter, const QColor& color)
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

    void clear()
    {
        scene.clear();
        rows = 0;
        anim_items.clear();
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
    }

    clear();

    if ( document )
    {
        connect(document->main_composition()->animation.get(), &model::AnimationContainer::first_frame_changed, this, &TimelineWidget::update_timeline_start);
        connect(document->main_composition()->animation.get(), &model::AnimationContainer::last_frame_changed, this, &TimelineWidget::update_timeline_end);
        update_timeline_end(document->main_composition()->animation->last_frame.get());
        update_timeline_start(document->main_composition()->animation->first_frame.get());
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
    // bg
    QPen dark(palette().color(QPalette::Text), 1);
    QPainter painter;

    // scene
    QGraphicsView::paintEvent(event);

    // fg
    painter.begin(viewport());


    if ( d->document )
    {
//         painter.begin(viewport());
        painter.setPen(dark);
        int cur_x = mapFromScene(d->document->current_time(), 0).x();
        painter.drawLine(cur_x, event->rect().top(), cur_x, event->rect().bottom());
//         painter.end();
    }

    painter.fillRect(event->rect().left(), 0, event->rect().right(), d->header_height,
                     palette().color(QPalette::Base));


    if ( d->document )
        d->paint_highligted_frame(d->document->current_time(), painter,
                                  palette().color(QPalette::Text));

    if ( d->mouse_frame > -1 )
        d->paint_highligted_frame(d->mouse_frame, painter, palette().color(QPalette::Highlight));

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
