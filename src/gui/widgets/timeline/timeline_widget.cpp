#include "timeline_widget.hpp"

#include <QScrollBar>
#include <QWheelEvent>
#include <QDebug>
#include <QTreeView>

#include "timeline_items.hpp"
#include "model/shapes/precomp_layer.hpp"
#include "model/shapes/styler.hpp"


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
    model::AnimationContainer* limit = nullptr;
    bool keep_highlight = false;

    LineItem* root = nullptr;
    std::unordered_map<quintptr, LineItem*> line_items;

    item_models::PropertyModelFull* base_model = nullptr;
    QAbstractItemModel* model = nullptr;
    QTreeView* expander = nullptr;

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

    LineItem* add_item(quintptr id, const item_models::PropertyModelFull::Item& item, LineItem* parent_item, int index)
    {
        LineItem* line_item = nullptr;

        if ( item.property )
            line_item = add_property(id, item.property);
        else if ( item.object )
            line_item = add_object_without_properties(id, item.object);

        if ( line_item )
        {
            line_items[id] = line_item;
            parent_item->add_row(line_item, index);
            connect(line_item, &LineItem::removed, parent, &TimelineWidget::on_item_removed);
        }

        return line_item;
    }

    LineItem* add_property(quintptr id, model::BaseProperty* prop)
    {
        LineItem* item;

        if ( prop->traits().flags & model::PropertyTraits::Animated )
            item = add_animatable(id, static_cast<model::AnimatableBase*>(prop));
        else if ( (prop->traits().flags & model::PropertyTraits::List) && prop->traits().is_object() )
            item = add_property_list(id, static_cast<model::ObjectListPropertyBase*>(prop));
        else
            item = add_property_plain(id, prop);

        return item;
    }

    LineItem* add_animatable(quintptr id, model::AnimatableBase* anim)
    {
        AnimatableItem* item = new AnimatableItem(id, anim->object(), anim, start_time, end_time, row_height);
        connect(item, &LineItem::clicked, parent, &TimelineWidget::line_clicked);
        return item;
    }

    LineItem* add_property_plain(quintptr id, model::BaseProperty* prop)
    {
        PropertyLineItem* item = new PropertyLineItem(id, prop->object(), prop, start_time, end_time, row_height);
        connect(item, &LineItem::clicked, parent, &TimelineWidget::line_clicked);
        return item;
    }

    LineItem* add_property_list(quintptr id, model::ObjectListPropertyBase* prop)
    {
        auto obj = prop->object();
        ObjectListLineItem* item = new ObjectListLineItem(id, obj, prop, start_time, end_time, row_height);
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
        connect(item, &LineItem::clicked, parent, &TimelineWidget::line_clicked);
        return item;
    }

    ObjectLineItem* add_object_without_properties(quintptr id, model::Object* obj)
    {
        ObjectLineItem* item = new ObjectLineItem(id, obj, start_time, end_time, row_height);
        if ( auto layer = obj->cast<model::PreCompLayer>() )
        {
            auto anim_item = new StretchableTimeItem(layer, row_height - 8, item);
            anim_item->setPos(0, row_height/2.0);
        }

        connect(item, &LineItem::clicked, parent, &TimelineWidget::line_clicked);

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
        root->set_time_end(end_time);
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
            return nullptr;
        return it->second;
    }

    void insert_index(const QModelIndex& parent_index, LineItem* parent, int index)
    {
        auto item = add_item(parent_index.internalId(), base_model->item(parent_index), parent, index);
        insert_children(parent_index, item);
    }

    void insert_children(const QModelIndex& parent_index, LineItem* parent_item)
    {
        int row_count = model->rowCount(parent_index);
        for ( int i = 0; i < row_count; i++ )
            insert_index(model->index(i, 0, parent_index), parent_item, i);
    }

    /**
     * \brief QTreeView isn't reliable with expanded/collapsed signals, so we check every time ;_;
     */
    void adjust_expand(const QModelIndex& parent_index, LineItem* parent_item)
    {
        int row_count = model->rowCount(parent_index);
        for ( int i = 0; i < row_count; i++ )
        {
            QModelIndex index = model->index(i, 0, parent_index);
            auto row = parent_item->rows()[i];
            row->set_expanded(expander->isExpanded(index));

        }
    }
};

TimelineWidget::TimelineWidget(QWidget* parent)
    : QGraphicsView(parent), d(std::make_unique<Private>())
{
    d->parent = this;
    d->root = new LineItem(0, nullptr, 0, 0, d->row_height);
    d->root->setPos(0, -d->row_height);
    d->root->expand();
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

    d->clear();
    d->document = document;

    if ( document )
    {
        connect(document->main()->animation.get(), &model::AnimationContainer::first_frame_changed, this, &TimelineWidget::update_timeline_start);
        connect(document->main()->animation.get(), &model::AnimationContainer::last_frame_changed, this, &TimelineWidget::update_timeline_end);
        update_timeline_end(document->main()->animation->last_frame.get());
        update_timeline_start(document->main()->animation->first_frame.get());
        connect(this, &TimelineWidget::frame_clicked, document, &model::Document::set_current_time);
        connect(document, &model::Document::current_time_changed, viewport(), (void (QWidget::*)())&QWidget::update);
    }

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
        {
            int row = (verticalScrollBar()->value() + d->header_height) / d->row_height;
            if ( event->angleDelta().y() > 0 && !event->inverted() )
                row -= 1;
            else
                row += 1;
            emit scrolled(row);
        }
        else
        {
            QApplication::sendEvent(horizontalScrollBar(), event);
        }
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
    QPointF viewport_scene_tl = mapFromScene(QPointF(0, 0));
    auto layer_top_left = mapFromScene(d->start_time, scene_tl.y());
    auto layer_top_right = mapFromScene(d->end_time, scene_tl.y());
    std::array<QBrush, 2> brushes = {
        palette().base(),
        palette().alternateBase(),
    };

    int n_rows = d->root->visible_rows();

    for ( int i = 0; i < n_rows - 1; i++ )
    {
        painter.fillRect(
            QRectF(
                QPointF(layer_top_left.x(), i*d->row_height + viewport_scene_tl.y()),
                QPointF(layer_top_right.x(), (i+1)*d->row_height + viewport_scene_tl.y())
            ),
            brushes[i%2]
        );
    }

    // gray out the area after the outside the layer range
    if ( layer_top_left.x() > 0 )
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
            QPointF(mapFromScene(d->start_time, 0).x(), 0),
            QPointF(mapFromScene(d->end_time+1, 0).x(), small_height)
        ),
        palette().base()
    );
    painter.fillRect(
        QRectF(
            QPointF(mapFromScene(d->time_round_to_ticks(d->start_time) - d->frame_skip, 0).x(), small_height),
            QPointF(mapFromScene(d->time_round_to_ticks(d->end_time), 0).x(), d->header_height)
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

        d->paint_frame_rect(this, painter, d->start_time, palette().base(), dark);

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
    // scene_rect changes min_scale, which changes tranform, which changes frame_skip, which changes scene_rect
    setSceneRect(d->scene_rect());
    d->adjust_min_scale(viewport()->width());

    d->update_frame_skip(QTransform::fromScale(d->min_scale, 1));

    setSceneRect(d->scene_rect());
    d->adjust_min_scale(viewport()->width());
    setTransform(QTransform::fromScale(d->min_scale, 1));
}

void TimelineWidget::resizeEvent(QResizeEvent* event)
{
    setSceneRect(d->scene_rect());
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

void TimelineWidget::select(const QItemSelection& selected, const QItemSelection& deselected)
{
    for ( const auto& index : selected.indexes() )
    {
        if ( auto item = d->index_to_line(index) )
        {
            item->setVisible(true);
            item->setSelected(true);
        }
    }

    for ( const auto& index : deselected.indexes() )
    {
        if ( auto item = d->index_to_line(index) )
            item->setSelected(false);
    }
}


item_models::PropertyModelFull::Item TimelineWidget::item_at(const QPoint& viewport_pos)
{
    for ( QGraphicsItem* it : items(viewport_pos) )
    {
        switch ( ItemTypes(it->type()) )
        {
            case ItemTypes::LineItem:
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
    setSceneRect(d->scene_rect());
    viewport()->update();
}

void TimelineWidget::expand(const QModelIndex& index)
{
    if ( auto line = d->index_to_line(index) )
        line->expand();
    setSceneRect(d->scene_rect());
    viewport()->update();
}

void TimelineWidget::set_model(QAbstractItemModel* model, item_models::PropertyModelFull* base_model, QTreeView* expander)
{
    d->model = model;
    d->base_model = base_model;
    d->expander = expander;
    connect(model, &QAbstractItemModel::rowsInserted, this, &TimelineWidget::model_rows_added);
    connect(model, &QAbstractItemModel::rowsRemoved, this, &TimelineWidget::model_rows_removed);
    connect(model, &QAbstractItemModel::rowsMoved, this, &TimelineWidget::model_rows_moved);
    connect(model, &QAbstractItemModel::modelReset, this, &TimelineWidget::model_reset);
}

void TimelineWidget::model_reset()
{
    d->clear();
    d->insert_children(QModelIndex(), d->root);
}

void TimelineWidget::model_rows_added(const QModelIndex& parent, int first, int last)
{
    auto parent_line = d->index_to_line(parent);
    if ( !parent_line )
        return;

    for ( int i = first; i <= last; i++ )
        d->insert_index(d->model->index(first, 0, parent), parent_line, i);


    d->adjust_expand(parent, parent_line);

    setSceneRect(d->scene_rect());

    // We are better at preserving selection than the treeview, so force a click
    emit_clicked();
}

void TimelineWidget::model_rows_removed(const QModelIndex& parent, int first, int last)
{
    auto line = d->index_to_line(parent);
    // Shouldn't happen but fail safe
    if ( !line || line == d->root )
        return;

    line->remove_rows(first, last);

    d->adjust_expand(parent, line);

    setSceneRect(d->scene_rect());

    // We are better at preserving selection than the treeview, so force a click
    emit_clicked();
}

void TimelineWidget::on_item_removed(quintptr id)
{
    d->line_items.erase(id);
}

void TimelineWidget::model_rows_moved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row)
{
    // Given how the property model does things, I can assume
    // start == end && parent == destination && row != start

    Q_UNUSED(end);
    Q_UNUSED(destination);

    if ( row > start )
        row -= 1;

    if ( LineItem* item = d->index_to_line(parent) )
    {
        item->move_row(start, row);

        d->adjust_expand(parent, item);
    }

    setSceneRect(d->scene_rect());

    // We are better at preserving selection than the treeview, so force a click
    emit_clicked();
}

static void debug_line(timeline::LineItem* line_item, QString indent, int index, bool recursive)
{

    QString item_name = line_item->metaObject()->className();
    item_name = item_name.mid(item_name.indexOf("::")+2);

    int effective_index = line_item->pos().y() / line_item->row_height();


    QString debug_string;
    auto item = line_item->property_item();
    if ( item.object )
    {
        debug_string = item.object->object_name();
        if ( debug_string.contains(' ') )
            debug_string = '"' + debug_string + '"';
    }

    if ( item.property )
    {
        if ( !debug_string.isEmpty() )
            debug_string += "->";
        debug_string += item.property->name();
    }

    if ( debug_string.isEmpty() )
        debug_string = "NULL";

    qDebug().noquote() << indent
        << index << effective_index << line_item->visible_height() /  line_item->row_height()
        << line_item->id() << item_name << debug_string << line_item->is_expanded() << line_item->isVisible();

    if ( recursive )
    {
        for ( uint i = 0; i < line_item->rows().size(); i++ )
            debug_line(line_item->rows()[i], indent + "    ", i, true);
    }
}

void TimelineWidget::debug_lines() const
{
    qDebug() << "index" << "effective_index" << "visible_rows" << "id" << "item_class" << "object->property" << "expanded" << "visible";
    debug_line(d->root, "", 0, true);

    qDebug() << sceneRect() << d->scene_rect() << d->rounded_end_time();
}

void TimelineWidget::toggle_debug(bool debug)
{
    timeline::enable_debug = debug;
    viewport()->update();
}

void TimelineWidget::emit_clicked()
{
    for ( auto item : d->scene.selectedItems() )
    {
        if ( item->type() >= int(ItemTypes::LineItem) )
        {
            emit line_clicked(static_cast<LineItem*>(item)->id(), true, true);
            return;
        }
    }
}
