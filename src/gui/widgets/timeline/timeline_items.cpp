/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "timeline_items.hpp"

#include "command/undo_macro_guard.hpp"
#include "keyframe_transition_data.hpp"

using namespace glaxnimate::gui;
using namespace glaxnimate;

bool timeline::enable_debug = false;

    timeline::KeyframeSplitItem::KeyframeSplitItem(AnimatableItem* parent)
    : QGraphicsObject(parent),
    visual_node(parent->object()->cast<model::VisualNode>())
{
    setFlags(
        QGraphicsItem::ItemIsSelectable|
        QGraphicsItem::ItemIgnoresTransformations
    );
}

void timeline::KeyframeSplitItem::set_enter(model::KeyframeTransition::Descriptive enter)
{
    icon_enter = KeyframeTransitionData::data(enter, KeyframeTransitionData::Finish).icon();
    pix_enter = icon_enter.pixmap(icon_size);
    update();
}

void timeline::KeyframeSplitItem::set_exit(model::KeyframeTransition::Descriptive exit)
{
    icon_exit = KeyframeTransitionData::data(exit, KeyframeTransitionData::Start).icon();
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

void timeline::KeyframeSplitItem::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    if ( event->button() == Qt::LeftButton )
    {

        if ( event->modifiers() & Qt::AltModifier )
        {
            line()->cycle_keyframe_transition(time());
            return;
        }

        event->accept();
        bool multi_select = (event->modifiers() & (Qt::ControlModifier|Qt::ShiftModifier)) != 0;

        if ( multi_select && isSelected() )
        {
            setSelected(false);
            return;
        }

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

void timeline::KeyframeSplitItem::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    if ( (event->buttons() & Qt::LeftButton) && isSelected() && !(event->modifiers() & Qt::AltModifier) )
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

timeline::AnimatableItem* timeline::KeyframeSplitItem::line() const
{
    return static_cast<timeline::AnimatableItem*>(parentItem());
}

void timeline::KeyframeSplitItem::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    if ( event->button() == Qt::LeftButton && isSelected() && !(event->modifiers() & Qt::AltModifier) )
    {
        event->accept();

        if ( drag_start == x() )
            return;

        std::map<AnimatableItem*, std::vector<AnimatableItem::DragData>> items;
        for ( auto item : scene()->selectedItems() )
        {
            if ( auto kf = dynamic_cast<KeyframeSplitItem*>(item) )
            {

                if ( kf->drag_allowed() )
                    items[kf->line()].push_back({kf, kf->drag_start, kf->time()});

                kf->drag_end();
            }
        }

        if ( !items.empty() )
        {
            command::UndoMacroGuard guard(tr("Drag Keyframes"), line()->object()->document());

            for ( const auto& p : items )
                p.first->keyframes_dragged(p.second);
        }
    }
    else
    {
        QGraphicsObject::mouseReleaseEvent(event);
    }
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

void timeline::LineItem::click_selected(bool selected, bool replace_selection)
{
    Q_EMIT clicked(id_, selected, replace_selection);
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

void timeline::LineItem::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    event->accept();
}

void timeline::LineItem::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    if ( event->button() == Qt::LeftButton )
    {
        bool selected = true;
        bool multiple = event->modifiers() & Qt::ControlModifier;
        if ( !multiple )
            scene()->clearSelection();
        else
            selected = !isSelected();

        setSelected(selected);
        click_selected(selected, !multiple);

        event->accept();
    }
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
    Q_EMIT removed(id_);
}

void timeline::LineItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    if ( isSelected() )
        painter->fillRect(option->rect, widget->palette().highlight());

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

timeline::AnimatableItem::AnimatableItem(quintptr id, model::Object* obj, model::AnimatableBase* animatable, int time_start, int time_end, int height)
    : LineItem(id, obj, time_start, time_end, height),
    animatable(animatable)
{
    for ( int i = 0; i < animatable->keyframe_count(); i++ )
        add_keyframe(i);

    connect(animatable, &model::AnimatableBase::keyframe_added, this, &AnimatableItem::add_keyframe);
    connect(animatable, &model::AnimatableBase::keyframe_removed, this, &AnimatableItem::remove_keyframe);
    connect(animatable, &model::AnimatableBase::keyframe_updated, this, &AnimatableItem::update_keyframe);
}

std::pair<model::KeyframeBase*, model::KeyframeBase*> timeline::AnimatableItem::keyframes(KeyframeSplitItem* item)
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

int timeline::AnimatableItem::type() const
{
    return int(ItemTypes::AnimatableItem);
}

item_models::PropertyModelFull::Item timeline::AnimatableItem::property_item() const
{
    return {object(), animatable};
}

void timeline::AnimatableItem::add_keyframe(int index)
{
    model::KeyframeBase* kf = animatable->keyframe(index);
    if ( index == 0 && !kf_split_items.empty() )
        kf_split_items[0]->set_enter(kf->transition().after_descriptive());

    model::KeyframeBase* prev = index > 0 ? animatable->keyframe(index-1) : nullptr;
    auto item = new KeyframeSplitItem(this);
    item->setPos(kf->time(), row_height() / 2.0);
    item->set_exit(kf->transition().before_descriptive());
    item->set_enter(prev ? prev->transition().after_descriptive() : model::KeyframeTransition::Hold);
    kf_split_items.insert(kf_split_items.begin() + index, item);

    connect(kf, &model::KeyframeBase::transition_changed, this, &AnimatableItem::transition_changed);
}

void timeline::AnimatableItem::remove_keyframe(int index)
{
    delete kf_split_items[index];
    kf_split_items.erase(kf_split_items.begin() + index);
    if ( index < int(kf_split_items.size()) && index > 0 )
    {
        kf_split_items[index]->set_enter(animatable->keyframe(index-1)->transition().after_descriptive());
    }
}

void timeline::AnimatableItem::transition_changed(model::KeyframeTransition::Descriptive before, model::KeyframeTransition::Descriptive after)
{
    int index = animatable->keyframe_index(static_cast<model::KeyframeBase*>(sender()));
    if ( index == -1 )
        return;

    kf_split_items[index]->set_exit(before);


    index += 1;
    if ( index >= int(kf_split_items.size()) )
        return;

    kf_split_items[index]->set_enter(after);
}

void timeline::AnimatableItem::keyframes_dragged(const std::vector<DragData>& keyframe_items)
{
    for ( auto kf : keyframe_items )
    {
        int index = animatable->keyframe_index(kf.from);
        auto cmd = new command::MoveKeyframe(animatable, index, kf.to);
        kf.item->setSelected(false);
        animatable->object()->push_command(cmd);
    }


    for ( auto kf : keyframe_items )
    {
        int index = animatable->keyframe_index(kf.to);
        kf_split_items[index]->setSelected(true);
    }
}

void glaxnimate::gui::timeline::AnimatableItem::cycle_keyframe_transition(model::FrameTime time)
{
    int index = animatable->keyframe_index(time);
    auto kf = animatable->keyframe(index);
    if ( !kf )
        return;

    auto desc = index == 0 ? kf->transition().after_descriptive() : kf->transition().before_descriptive();

    switch ( desc )
    {
        case model::KeyframeTransition::Hold:
            desc = model::KeyframeTransition::Linear;
            break;
        case model::KeyframeTransition::Linear:
            desc = model::KeyframeTransition::Ease;
            break;
        case model::KeyframeTransition::Ease:
            desc = model::KeyframeTransition::Fast;
            break;
        case model::KeyframeTransition::Fast:
            desc = model::KeyframeTransition::Overshoot;
            break;
        case model::KeyframeTransition::Overshoot:
            desc = model::KeyframeTransition::Hold;
            break;
        case model::KeyframeTransition::Custom:
            desc = model::KeyframeTransition::Hold;
            break;
    }

    {
        command::UndoMacroGuard guard(tr("Update keyframe transition"), animatable->object()->document());
        if ( index > 0 )
        {
            auto kf_before = animatable->keyframe(index - 1);
            auto left_trans = kf_before->transition();
            left_trans.set_after_descriptive(desc);
            animatable->object()->push_command(new command::SetKeyframeTransition(animatable, index-1, left_trans));
        }

        auto right_trans = kf->transition();
        right_trans.set_before_descriptive(desc);
        animatable->object()->push_command(new command::SetKeyframeTransition(animatable, index, right_trans));
    }
}


void timeline::AnimatableItem::update_keyframe(int index, model::KeyframeBase* kf)
{
    auto item_start = kf_split_items[index];
    item_start->setPos(kf->time(), row_height() / 2.0);
    item_start->set_exit(kf->transition().before_descriptive());

    if ( index == 0 )
        item_start->set_enter(model::KeyframeTransition::Hold);

    if ( index < int(kf_split_items.size()) - 1 )
    {
        auto item_end = kf_split_items[index+1];
        item_end->set_enter(kf->transition().after_descriptive());
    }
}
