#pragma once

#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsObject>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>

#include "app/application.hpp"
#include "command/animation_commands.hpp"
#include "model/document.hpp"
#include "model/shapes/precomp_layer.hpp"
#include "graphics/handle.hpp"

namespace timeline {


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

enum class ItemTypes
{
    ObjectLineItem = QGraphicsItem::UserType + 1,
    AnimatableItem,
    PropertyLineItem,
};

class LineItem : public QGraphicsObject
{
    Q_OBJECT

public:
    LineItem(int time_start, int time_end, int height, int row)
        : time_start(time_start), time_end(time_end), height_(height), row_(row)
    {
        setFlags(QGraphicsItem::ItemIsSelectable);
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

//     void set_height(int h)
//     {
//         height_ = h;
//         on_height_changed(h);
//         prepareGeometryChange();
//     }

    QRectF boundingRect() const override
    {
        return QRectF(time_start, 0, time_end, height_);
    }

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) override;

    int height() const
    {
        return height_;
    }


protected:
    void mousePressEvent(QGraphicsSceneMouseEvent * event) override
    {
        bool sel = isSelected();
        QGraphicsObject::mousePressEvent(event);
        if ( !sel && isSelected() )
            click_selected();
    }

    virtual void click_selected(){}
//     virtual void on_height_changed(int){}


private:
    int time_start;
    int time_end;
    int height_;
    int row_;
};

class ObjectLineItem : public LineItem
{
    Q_OBJECT

public:
    ObjectLineItem(model::Object* obj, int time_start, int time_end, int height, int row)
        : LineItem(time_start, time_end, height, row), object_(obj)
    {}

    int type() const override { return int(ItemTypes::ObjectLineItem); }

    model::Object* object() const
    {
        return object_;
    }

protected:
    void click_selected() override
    {
        emit object_clicked(object_);
    }

signals:
    void object_clicked(model::Object* object);

private:
    model::Object* object_;
};


class AnimatableItem : public LineItem
{
    Q_OBJECT

public:
    AnimatableItem(model::AnimatableBase* animatable, int time_start, int time_end, int height, int row)
        : LineItem(time_start, time_end, height, row), animatable(animatable)
    {
        for ( int i = 0; i < animatable->keyframe_count(); i++ )
            add_keyframe(i);

        connect(animatable, &model::AnimatableBase::keyframe_added, this, &AnimatableItem::add_keyframe);
        connect(animatable, &model::AnimatableBase::keyframe_removed, this, &AnimatableItem::remove_keyframe);
        connect(animatable, &model::AnimatableBase::keyframe_updated, this, &AnimatableItem::update_keyframe);
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

    int type() const override { return int(ItemTypes::AnimatableItem); }

protected:
    void click_selected() override
    {
        emit animatable_clicked(animatable);
    }

public slots:
    void add_keyframe(int index)
    {
        model::KeyframeBase* kf = animatable->keyframe(index);
        if ( index == 0 && !kf_split_items.empty() )
            kf_split_items[0]->set_enter(kf->transition().after_descriptive());

        model::KeyframeBase* prev = index > 0 ? animatable->keyframe(index-1) : nullptr;
        auto item = new KeyframeSplitItem(this);
        item->setPos(kf->time(), height() / 2.0);
        item->set_exit(kf->transition().before_descriptive());
        item->set_enter(prev ? prev->transition().after_descriptive() : model::KeyframeTransition::Hold);
        kf_split_items.insert(kf_split_items.begin() + index, item);

        connect(kf, &model::KeyframeBase::transition_changed, this, &AnimatableItem::transition_changed);
        connect(item, &KeyframeSplitItem::dragged, this, &AnimatableItem::keyframe_dragged);
    }

    void remove_keyframe(int index)
    {
        delete kf_split_items[index];
        kf_split_items.erase(kf_split_items.begin() + index);
        if ( index < int(kf_split_items.size()) && index > 0 )
        {
            kf_split_items[index]->set_enter(animatable->keyframe(index-1)->transition().after_descriptive());
        }
    }

signals:
    void animatable_clicked(model::AnimatableBase* animatable);

private slots:
    void transition_changed(model::KeyframeTransition::Descriptive before, model::KeyframeTransition::Descriptive after)
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
        item_start->setPos(kf->time(), height() / 2.0);
        item_start->set_exit(kf->transition().before_descriptive());

        if ( index < int(kf_split_items.size()) - 1 )
        {
            auto item_end = kf_split_items[index];
            item_end->set_enter(kf->transition().after_descriptive());
        }
    }

public:
    model::AnimatableBase* animatable;
    std::vector<KeyframeSplitItem*> kf_split_items;
};

class TimeRectItem : public QGraphicsObject
{
public:
    TimeRectItem(model::DocumentNode* node, qreal height, QGraphicsItem* parent)
    : QGraphicsObject(parent),
      radius(height/2)
    {
        update_color(node->docnode_group_color());
        connect(node, &model::DocumentNode::docnode_group_color_changed, this, &TimeRectItem::update_color);
    }

    void paint(QPainter * painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        QPen p(stroke, 1);
        p.setCosmetic(true);
        painter->setPen(p);
        painter->setBrush(color);
        painter->drawRect(boundingRect());
    }

private:
    void update_color(const QColor& col)
    {
        color = col;
        color.setAlpha(255);
        if ( color.lightnessF() < 0.4 )
            stroke = Qt::white;
        else
            stroke = Qt::black;

        update();
    }

protected:
    QColor color;
    QColor stroke;
    qreal radius;
};

class AnimationContainerItem : public TimeRectItem
{
public:
    AnimationContainerItem(model::DocumentNode* node, model::AnimationContainer* animation,
                           qreal height, QGraphicsItem* parent)
    : TimeRectItem(node, height, parent),
      animation(animation)

    {
        handle_ip.set_radius(radius);
        handle_op.set_radius(radius);
        handle_ip.setPos(animation->first_frame.get(), 0);
        handle_op.setPos(animation->last_frame.get(), 0);
        connect(&handle_ip, &graphics::MoveHandle::dragged_x, this, &AnimationContainerItem::drag_ip);
        connect(&handle_op, &graphics::MoveHandle::dragged_x, this, &AnimationContainerItem::drag_op);
        connect(animation, &model::AnimationContainer::first_frame_changed, this, &AnimationContainerItem::update_ip);
        connect(animation, &model::AnimationContainer::last_frame_changed, this, &AnimationContainerItem::update_op);
        connect(&handle_ip, &graphics::MoveHandle::drag_finished, this, &AnimationContainerItem::commit_ip);
        connect(&handle_op, &graphics::MoveHandle::drag_finished, this, &AnimationContainerItem::commit_op);
    }

    QRectF boundingRect() const override
    {
        return QRectF(
            QPointF(handle_ip.pos().x(), -radius),
            QPointF(handle_op.pos().x(), radius)
        );
    }

private:
    void drag_ip(qreal x)
    {
        x = qRound(x);
        if ( x >= animation->last_frame.get() )
            x = animation->last_frame.get() - 1;
        animation->first_frame.set_undoable(x, false);
    }

    void drag_op(qreal x)
    {
        x = qRound(x);
        if ( x <= animation->first_frame.get() )
            x = animation->first_frame.get() + 1;
        animation->last_frame.set_undoable(x, false);
    }

    void update_ip(qreal x)
    {
        handle_ip.setPos(x, 0);
        prepareGeometryChange();
        update();
    }

    void update_op(qreal x)
    {
        handle_op.setPos(x, 0);
        prepareGeometryChange();
        update();
    }

    void commit_ip()
    {
        animation->first_frame.set_undoable(animation->first_frame.get(), true);
    }

    void commit_op()
    {
        animation->last_frame.set_undoable(animation->last_frame.get(), true);
    }

private:
    graphics::MoveHandle handle_ip{this, graphics::MoveHandle::Horizontal, graphics::MoveHandle::None, 1, true};
    graphics::MoveHandle handle_op{this, graphics::MoveHandle::Horizontal, graphics::MoveHandle::None, 1, true};
    model::AnimationContainer* animation;
};


class PropertyLineItem : public LineItem
{
    Q_OBJECT

public:
    PropertyLineItem(model::BaseProperty* obj, int time_start, int time_end, int height, int row)
        : LineItem(time_start, time_end, height, row), property_(obj)
    {}

    int type() const override { return int(ItemTypes::PropertyLineItem); }

    model::BaseProperty* property() const
    {
        return property_;
    }

protected:
    void click_selected() override
    {
        emit property_clicked(property_);
    }

signals:
    void property_clicked(model::BaseProperty* object);

private:
    model::BaseProperty* property_;
};


class StretchableTimeItem : public TimeRectItem
{
public:
    StretchableTimeItem(model::PreCompLayer* layer,
                           qreal height, QGraphicsItem* parent)
    : TimeRectItem(layer, height, parent),
      timing(layer->timing.get()),
      animation(timing->document()->main()->animation.get())

    {
        handle_ip.set_radius(radius);
        handle_op.set_radius(radius);
        update_handles();
        connect(&handle_ip, &graphics::MoveHandle::dragged_x,               this, &StretchableTimeItem::drag_ip);
        connect(&handle_op, &graphics::MoveHandle::dragged_x,               this, &StretchableTimeItem::drag_op);
        connect(timing,     &model::StretchableTime::timing_changed,        this, &StretchableTimeItem::update_handles);
        connect(layer,      &model::PreCompLayer::composition_changed,      this, &StretchableTimeItem::update_handles);
        connect(&handle_ip, &graphics::MoveHandle::drag_finished,           this, &StretchableTimeItem::commit_ip);
        connect(&handle_op, &graphics::MoveHandle::drag_finished,           this, &StretchableTimeItem::commit_op);
        connect(animation,  &model::AnimationContainer::last_frame_changed, this, &StretchableTimeItem::update_handles);

    }

    QRectF boundingRect() const override
    {
        return QRectF(
            QPointF(handle_ip.pos().x(), -radius),
            QPointF(handle_op.pos().x(), radius)
        );
    }

private:
    void drag_ip(qreal x)
    {
        x = qRound(x);
        timing->start_time.set_undoable(x, false);
    }

    void drag_op(qreal x)
    {
        x = qRound(x);
        auto duration = x - timing->start_time.get();
        if ( duration < 1 )
            duration = 1;

        auto source_duration = animation->last_frame.get();

        if ( source_duration != 0 )
            timing->stretch.set_undoable(duration / source_duration, false);
    }

    void update_ip(qreal x)
    {
        handle_ip.setPos(x, 0);
        prepareGeometryChange();
        update();
    }

    void update_handles()
    {
        handle_ip.setPos(timing->start_time.get(), 0);

        handle_op.setPos(timing->start_time.get() + animation->last_frame.get() * timing->stretch.get(), 0);
        prepareGeometryChange();
        update();
    }

    void commit_ip()
    {
        timing->start_time.set_undoable(timing->start_time.get(), true);
    }

    void commit_op()
    {
        timing->stretch.set_undoable(timing->stretch.get(), true);
    }

private:
    model::StretchableTime* timing;
    model::AnimationContainer* animation;
    graphics::MoveHandle handle_ip{this, graphics::MoveHandle::Horizontal, graphics::MoveHandle::None, 1, true};
    graphics::MoveHandle handle_op{this, graphics::MoveHandle::Horizontal, graphics::MoveHandle::None, 1, true};
};


} // namespace timeline
