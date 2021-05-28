#include "timeline_items.hpp"


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
    adjust_row_vis(row->visible_rows(), row);
}

void timeline::LineItem::remove_rows(int first, int last)
{
    /// \todo Figure out why these can occur
    if ( first >= int(rows_.size()) )
        return;
    if ( last >= int(rows_.size()) )
        last = rows_.size() - 1;

    int delta = 0;
    LineItem* row = nullptr;
    for ( int i = first; i <= last && i < int(rows_.size()); i++ )
    {
        row = rows_[i];
        row->emit_removed();
        delta -= row->visible_rows();
        delete row;
    }
    rows_.erase(rows_.begin() + first, rows_.begin() + last + 1);
    adjust_row_vis(delta, rows_[first]);
}

void timeline::LineItem::move_row(int from, int to)
{
    LineItem* row = rows_[from];
    int delta = row->visible_rows();

    adjust_row_vis(-delta, row);
    rows_.erase(rows_.begin() + from);

    rows_.insert(rows_.begin() + to, row);
    adjust_row_vis(delta, row, true);
}

void timeline::LineItem::expand()
{
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
    int old_vis = visible_rows_;
    for ( auto item : rows_ )
        item->setVisible(false);

    visible_rows_ = 1;
    propagate_row_vis(visible_rows_ - old_vis);

    expanded_ = false;
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
        parent_line()->adjust_row_vis(delta, this);
}

void timeline::LineItem::adjust_row_vis(int delta, LineItem* child, bool adjust_child)
{
    if ( expanded_ )
        visible_rows_ += delta;

    int i;
    for ( i = 0; i < int(rows_.size()); i++ )
    {
        if ( rows_[i] == child )
            break;
    }

    if ( !adjust_child )
        i++;

    for ( ; i < int(rows_.size()); i++ )
    {
        rows_[i]->setPos(0, rows_[i]->pos().y() + row_height() * delta);
    }

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
    /*
    painter->save();
    painter->setBrush(Qt::black);
    painter->scale(0.5, 1);
    QFont f;
    painter->setFont(f);

    QString debug_string = metaObject()->className();
    debug_string = debug_string.mid(debug_string.indexOf("::")+2);
    painter->drawText(0, row_height(), debug_string);

    auto item = property_item();
    if ( item.property )
        debug_string = item.property->name();
    else if ( item.object )
        debug_string = item.object->object_name();
    else
        debug_string = "NULL";
    painter->drawText(time_end, row_height(), debug_string);

    painter->restore();
    */
}
