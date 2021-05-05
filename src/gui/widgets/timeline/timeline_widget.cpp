#include "timeline_widget.hpp"

#include <QScrollBar>
#include <QWheelEvent>

#include "timeline_items.hpp"
#include "model/shapes/precomp_layer.hpp"
#include <model/shapes/styler.hpp>

using namespace timeline;

class TimelineWidget::Private
{
public:
    TimelineWidget* parent;
    QGraphicsScene scene;
    int start_time = 0;
    int end_time = 0;
    int row_height = 24;
    int header_height = 24;

    qreal min_scale = 1;
    int frame_skip = 1;
    int min_gap = 32;
    int mouse_frame = -1;
    model::Document* document = nullptr;
    bool dragging_frame = false;
    int layer_start = 0;
    int layer_end = 0;
    model::AnimationContainer* limit = nullptr;
    bool keep_highlight = false;

    LineItem* root = nullptr;
    std::unordered_map<quintptr, LineItem*> line_items;

    item_models::PropertyModelFull* model = nullptr;

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
            QPointF(rounded_end_time(), std::max(row_height*root->visible_rows(), parent->height()))
        );
    }

    void add_item(quintptr id, const item_models::PropertyModelFull::Item& item, LineItem* parent_item)
    {
        LineItem* line_item = nullptr;

        if ( item.property )
            line_item = add_property(id, item.property, parent_item);
        else if ( item.object )
            line_item = add_object_without_properties(id, item.object);

        if ( line_item )
        {
            line_items[id] = line_item;
            parent_item->add_row(line_item);
            connect(line_item, &LineItem::removed, parent, &TimelineWidget::on_item_removed);
        }
    }

    LineItem* add_property(quintptr id, model::BaseProperty* prop, LineItem* parent_item)
    {
        LineItem* item;

        if ( prop->traits().flags & model::PropertyTraits::Animated )
            item = add_animatable(id, static_cast<model::AnimatableBase*>(prop), parent_item);
        else if ( (prop->traits().flags & model::PropertyTraits::List) && prop->traits().is_object() )
            item = add_property_list(id, static_cast<model::ObjectListPropertyBase*>(prop), parent_item);
        else
            item = add_property_plain(id, prop, parent_item);

        return item;
    }

    LineItem* add_animatable(quintptr id, model::AnimatableBase* anim, LineItem* parent_item)
    {
        AnimatableItem* item = new AnimatableItem(id, parent_item->object(), anim, start_time, rounded_end_time(), row_height);
        connect(item, &AnimatableItem::animatable_clicked, parent, &TimelineWidget::property_clicked);
        return item;
    }

    LineItem* add_property_plain(quintptr id, model::BaseProperty* prop, LineItem* parent_item)
    {
        PropertyLineItem* item = new PropertyLineItem(id, parent_item->object(), prop, start_time, rounded_end_time(), row_height);
        connect(item, &PropertyLineItem::property_clicked, parent, &TimelineWidget::property_clicked);
        return item;
    }

    LineItem* add_property_list(quintptr id, model::ObjectListPropertyBase* prop, LineItem* parent_item)
    {
        ObjectListLineItem* item = new ObjectListLineItem(id, parent_item->object(), prop, start_time, rounded_end_time(), row_height);
        connect(item, &ObjectListLineItem::property_clicked, parent, &TimelineWidget::property_clicked);
        return item;
    }

    ObjectLineItem* add_object_without_properties(quintptr id, model::Object* obj)
    {
        ObjectLineItem* item = new ObjectLineItem(id, obj, start_time, rounded_end_time(), row_height);
        if ( auto layer = obj->cast<model::Layer>() )
        {
            auto anim_item = new AnimationContainerItem(layer, layer->animation.get(), row_height - 8, item);
            anim_item->setPos(0, row_height/2.0);
        }
        else if ( auto comp = obj->cast<model::MainComposition>() )
        {
            auto anim_item = new AnimationContainerItem(comp, comp->animation.get(), row_height - 8, item);
            anim_item->setPos(0, row_height/2.0);
        }
        else if ( auto layer = obj->cast<model::PreCompLayer>() )
        {
            auto anim_item = new StretchableTimeItem(layer, row_height - 8, item);
            anim_item->setPos(0, row_height/2.0);
        }

        connect(item, &ObjectLineItem::object_clicked, parent, &TimelineWidget::object_clicked);

        return item;
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
        root->set_time_end(et);
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
        line_items.clear();
        root->raw_clear();
    }

    model::AnimationContainer* anim(model::DocumentNode* node)
    {
        while ( node )
        {
            const QMetaObject* mo = node->metaObject();
            if ( mo->inherits(&model::Layer::staticMetaObject) )
                return static_cast<model::Layer*>(node)->animation.get();
            else if ( mo->inherits(&model::MainComposition::staticMetaObject) )
                return static_cast<model::MainComposition*>(node)->animation.get();

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

    LineItem* index_to_line(const QModelIndex& index)
    {
        auto it = line_items.find(index.internalId());
        if ( it == line_items.end() )
            return root;
        return it->second;
    }

    void insert_index(const QModelIndex& index, LineItem* parent)
    {
        add_item(index.internalId(), model->item(index), parent);
    }
};

TimelineWidget::TimelineWidget(QWidget* parent)
    : QGraphicsView(parent), d(std::make_unique<Private>())
{
    d->parent = this;
    d->root = new LineItem(0, nullptr, 0, 0, d->row_height);
    d->scene.addItem(d->root);
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
/*
void TimelineWidget::add_object(model::Object* node)
{
    d->add_object_without_properties(node);
}

void TimelineWidget::add_property(model::BaseProperty* property)
{
    if ( !d->object_items.empty() )
    {
        if ( property->traits().flags & model::PropertyTraits::Animated )
            d->add_animatable(static_cast<model::AnimatableBase*>(property), d->object_items.back());
        else
            d->add_property(property, d->object_items.back());
    }
}

void TimelineWidget::remove_object(model::Object* obj)
{
    for ( auto i = d->prop_items.begin(); i != d->prop_items.end(); )
    {
        if ( i->second->object() == obj )
            i = d->prop_items.erase(i);
        else
            ++i;
    }

    auto it = d->find_object_item(obj);
    if ( it == d->object_items.end() )
        return;

    qreal delta;
    if ( (*it)->is_expanded() )
        delta = (*it)->rows_height();
    else
        delta = (*it)->height();

    disconnect(*it, nullptr, this, nullptr);
    delete *it;
    it = d->object_items.erase(it);
    for ( ; it != d->object_items.end(); ++it )
    {
        auto p = (*it)->pos();
        p.setY(p.y() - delta);
        (*it)->setPos(p);
    }

    setSceneRect(d->scene_rect());
}

void TimelineWidget::clear()
{
    for ( auto objit : d->object_items )
        disconnect(objit, nullptr, this, nullptr);
    d->clear();
    setSceneRect(d->scene_rect());
    reset_view();
}
*/
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

//     clear();

    if ( document )
    {
        connect(document->main()->animation.get(), &model::AnimationContainer::first_frame_changed, this, &TimelineWidget::update_timeline_start);
        connect(document->main()->animation.get(), &model::AnimationContainer::last_frame_changed, this, &TimelineWidget::update_timeline_end);
        update_timeline_end(document->main()->animation->last_frame.get());
        update_timeline_start(document->main()->animation->first_frame.get());
        connect(this, &TimelineWidget::frame_clicked, document, &model::Document::set_current_time);
        connect(document, &model::Document::current_time_changed, viewport(), (void (QWidget::*)())&QWidget::update);
//         set_anim_container(document->main()->animation.get());
    }
    else
    {
//         set_anim_container(nullptr);
    }

    d->document = document;
}

void TimelineWidget::update_timeline_end(model::FrameTime end)
{
    d->end_time = end;
    setSceneRect(sceneRect() | d->scene_rect());
    d->adjust_min_scale(viewport()->width());
    d->update_end_time();
}

void TimelineWidget::update_timeline_start(model::FrameTime start)
{
    d->start_time = start;
    setSceneRect(sceneRect() | d->scene_rect());
    d->adjust_min_scale(viewport()->width());
    d->root->set_time_start(qFloor(start));
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

    // stripy rows
    auto layer_top_left = mapFromScene(d->layer_start, scene_tl.y());
    auto layer_top_right = mapFromScene(d->layer_end, scene_tl.y());
    std::array<QBrush, 2> brushes = {
        palette().alternateBase(),
        palette().base(),
    };

    int n_rows = d->root->visible_rows();

    for ( int i = 0; i <= n_rows; i++ )
    {
        painter.fillRect(
            QRectF(
                QPointF(layer_top_left.x(), i*d->row_height),
                QPointF(layer_top_right.x(), (i+1)*d->row_height)
            ),
            brushes[i%2]
        );
    }

    // gray out the area after the outside the layer range
    if ( d->layer_start > d->start_time )
    {
        painter.fillRect(
            QRectF(
                QPointF(0, event->rect().top()),
                QPointF(layer_top_left.x(), event->rect().bottom())
            ),
            disabled
        );
    }
    painter.fillRect(
        QRectF(
            layer_top_right,
            QPointF(width(), event->rect().right())
        ),
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
    setSceneRect(d->scene_rect());
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
        auto selection = d->scene.selectedItems();
        QGraphicsView::mousePressEvent(event);
        if ( d->scene.selectedItems().empty() )
            for ( const auto& s : selection )
                s->setSelected(true);
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
    if ( !d->keep_highlight )
    {
        d->mouse_frame = -1;
        viewport()->update();
    }
    d->keep_highlight = false;
}

void TimelineWidget::enterEvent(QEvent* event)
{
    QGraphicsView::enterEvent(event);
    d->keep_highlight = false;
}

int TimelineWidget::header_height() const
{
    return d->header_height;
}

void TimelineWidget::select(const QModelIndex& index)
{
    d->scene.clearSelection();
    auto it = d->line_items.find(index.internalId());
    if ( it != d->line_items.end() )
        it->second->setSelected(true);
}

item_models::PropertyModelFull::Item TimelineWidget::item_at(const QPoint& viewport_pos)
{
    for ( QGraphicsItem* it : items(viewport_pos) )
    {
        switch ( ItemTypes(it->type()) )
        {
            case ItemTypes::AnimatableItem:
            case ItemTypes::ObjectLineItem:
            case ItemTypes::PropertyLineItem:
            case ItemTypes::ObjectListLineItem:
                return static_cast<LineItem*>(it)->property_item();
        }
    }
    return {};
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

/*
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
*/

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

qreal TimelineWidget::highlighted_time() const
{
    if ( d->mouse_frame == -1 && d->document )
        return d->document->current_time();

    return d->mouse_frame;
}

void TimelineWidget::set_highlighted_time(int time)
{
    d->mouse_frame = time;
}

void TimelineWidget::keep_highlight()
{
    d->keep_highlight = true;
}

void TimelineWidget::collapse(const QModelIndex& index)
{
    if ( auto line = d->index_to_line(index) )
        line->collapse();
}

void TimelineWidget::expand(const QModelIndex& index)
{
    if ( auto line = d->index_to_line(index) )
        line->expand();
}

void TimelineWidget::set_model(item_models::PropertyModelFull* model)
{
    d->model = model;
    connect(model, &item_models::PropertyModelFull::rowsInserted, this, &TimelineWidget::model_rows_added);
    connect(model, &item_models::PropertyModelFull::rowsRemoved, this, &TimelineWidget::model_rows_removed);
    connect(model, &item_models::PropertyModelFull::modelReset, this, &TimelineWidget::model_reset);
}

void TimelineWidget::model_reset()
{
    d->clear();
}

void TimelineWidget::model_rows_added(const QModelIndex& parent, int first, int last)
{
    auto parent_line = d->index_to_line(parent);
    for ( int i = first; i <= last; i++ )
        d->insert_index(d->model->index(first, 0, parent), parent_line);
    setSceneRect(d->scene_rect());
}

void TimelineWidget::model_rows_removed(const QModelIndex& parent, int first, int last)
{
    d->index_to_line(parent)->remove_rows(first, last);
    setSceneRect(d->scene_rect());
}

void TimelineWidget::on_item_removed(quintptr id)
{
    d->line_items.erase(id);
}

