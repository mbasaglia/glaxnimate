#include "timeline_items.hpp"

bool timeline::enable_debug = false;


void timeline::KeyframeSplitItem::set_enter(model::KeyframeTransition::Descriptive enter)
{
    icon_enter = icon_from_kdf(enter, "finish");
    pix_enter = icon_enter.pixmap(icon_size);
    update();
}

void timeline::KeyframeSplitItem::set_exit(model::KeyframeTransition::Descriptive exit)
{
    icon_exit = icon_from_kdf(exit, "start");
    pix_exit = icon_exit.pixmap(icon_size);
    update();
}

void timeline::KeyframeSplitItem::paint(QPainter * painter, const QStyleOptionGraphicsItem *, QWidget * widget)
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

    painter->drawPixmap(-icon_size/2, -icon_size/2, half_icon_size.width(), half_icon_size.height(), pix_enter);
    painter->drawPixmap(0, -icon_size/2, half_icon_size.width(), half_icon_size.height(), pix_exit);
}


timeline::LineItem::LineItem(quintptr id, model::Object* obj, int time_start, int time_end, int height):
    time_start(time_start),
    time_end(time_end),
    height_(height),
    object_(obj),
    id_(id)
{
    setFlags(QGraphicsItem::ItemIsSelectable);
}

void timeline::LineItem::click_selected()
{
    emit clicked(id_);
}


model::Object* timeline::LineItem::object() const
{
    return object_;
}

void timeline::LineItem::set_time_start(int time)
{
    time_start = time;
    for ( auto row : rows_ )
        row->set_time_start(time);
    on_set_time_start(time);
    prepareGeometryChange();
}

void timeline::LineItem::set_time_end(int time)
{
    time_end = time;
    for ( auto row : rows_ )
        row->set_time_end(time);
    on_set_time_end(time);
    prepareGeometryChange();
}

QRectF timeline::LineItem::boundingRect() const
{
    return QRectF(time_start, 0, time_end, height_);
}

int timeline::LineItem::row_height() const
{
    return height_;
}

int timeline::LineItem::row_count() const
{
    return rows_.size();
}

int timeline::LineItem::visible_height() const
{
    return visible_rows_ * row_height();
}

void timeline::LineItem::add_row(LineItem* row, int index)
{
    row->setParentItem(this);
    row->setPos(0, height_ * (index+1));
    rows_.insert(rows_.begin() + index, row);
    if ( !expanded_ )
        row->setVisible(false);
    adjust_row_vis(row->visible_rows());
}

void timeline::LineItem::remove_rows(int first, int last)
{
    /// \todo Figure out why these can occur
    if ( first >= int(rows_.size()) )
        return;
    if ( last >= int(rows_.size()) )
        last = rows_.size() - 1;

    int delta = 0;
    for ( int i = first; i <= last && i < int(rows_.size()); i++ )
    {
        LineItem* row = rows_[i];
        row->emit_removed();
        delta -= row->visible_rows();
        delete row;
    }
    rows_.erase(rows_.begin() + first, rows_.begin() + last + 1);
    adjust_row_vis(delta);
}

void timeline::LineItem::move_row(int from, int to)
{
    LineItem* row = rows_[from];
    int delta = row->visible_rows();

    rows_.erase(rows_.begin() + from);
    rows_.insert(rows_.begin() + to, row);

    adjust_row_vis(-delta, false);
}

void timeline::LineItem::expand()
{
    if ( expanded_ )
        return;

    expanded_ = true;

    int old_vis = visible_rows_;
    visible_rows_ = 1;

    for ( auto item : rows_ )
    {
        item->setVisible(true);
        visible_rows_ += item->visible_rows();
    }

    propagate_row_vis(visible_rows_ - old_vis);
}

void timeline::LineItem::collapse()
{
    if ( !expanded_ )
        return;

    int old_vis = visible_rows_;
    for ( auto item : rows_ )
        item->setVisible(false);

    visible_rows_ = 1;
    propagate_row_vis(visible_rows_ - old_vis);

    expanded_ = false;
}

void timeline::LineItem::set_expanded(bool expanded)
{
    if ( expanded != expanded_ )
    {
        if ( expanded )
            expand();
        else
            collapse();
    }
}


bool timeline::LineItem::is_expanded()
{
    return expanded_;
}

timeline::LineItem* timeline::LineItem::parent_line() const
{
    return static_cast<LineItem*>(parentItem());
}

int timeline::LineItem::visible_rows() const
{
    return visible_rows_;
}

void timeline::LineItem::raw_clear()
{
    for ( auto row : rows_ )
        delete row;
    rows_.clear();
    visible_rows_ = 1;
}

void timeline::LineItem::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    bool sel = isSelected();
    QGraphicsObject::mousePressEvent(event);
    if ( !sel && isSelected() )
        click_selected();
}

void timeline::LineItem::propagate_row_vis(int delta)
{
    if ( isVisible() && parent_line() && delta && expanded_ )
        parent_line()->adjust_row_vis(delta);
}

void timeline::LineItem::adjust_row_vis(int delta, bool propagate)
{
    if ( expanded_ )
        visible_rows_ += delta;

    int y = 1;
    for ( auto row : rows_ )
    {
        row->setPos(0, height_ * y);
        y += row->visible_rows_;
    }

    if ( propagate )
        propagate_row_vis(delta);
}

void timeline::LineItem::emit_removed()
{
    for ( auto row : rows_ )
        row->emit_removed();
    emit removed(id_);
}

void timeline::LineItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    if ( isSelected() )
        painter->fillRect(option->rect, widget->palette().highlight());
//     else if ( isUnderMouse() )
//         painter->fillRect(option->rect, widget->palette().brush(QPalette::Inactive, QPalette::Highlight));

    // Debugging print, it shows some meta info on the line, useful when tweaking the layout
    if ( enable_debug )
    {
        painter->save();
        painter->setBrush(Qt::black);
        painter->scale(0.5, 1);
        QFont f;
        painter->setFont(f);

        QString debug_string = metaObject()->className();
        debug_string = debug_string.mid(debug_string.indexOf("::")+2);
        painter->drawText(0, row_height(), debug_string);

        painter->drawText(time_end * 3/4, row_height(), QString::number(id_));

        auto item = property_item();
        if ( item.property )
            debug_string = item.property->name();
        else if ( item.object )
            debug_string = item.object->object_name();
        else
            debug_string = "NULL";
        painter->drawText(time_end, row_height(), debug_string);

        painter->restore();
    }
}
