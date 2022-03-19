#pragma once

#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsObject>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>

#include "app/application.hpp"
#include "glaxnimate/core/command/animation_commands.hpp"
#include "glaxnimate/core/model/document.hpp"
#include "glaxnimate/core/model/shapes/precomp_layer.hpp"

#include "graphics/handle.hpp"
#include "item_models/property_model_full.hpp"

namespace glaxnimate::gui::timeline {

extern bool enable_debug;

enum class ItemTypes
{
    LineItem = QGraphicsItem::UserType + 1,
    ObjectLineItem,
    AnimatableItem,
    PropertyLineItem,
    ObjectListLineItem,
};

class LineItem : public QGraphicsObject
{
    Q_OBJECT

public:
    LineItem(quintptr id, model::Object* obj, int time_start, int time_end, int height);

    model::Object* object() const;

    void set_time_start(int time);

    void set_time_end(int time);

    QRectF boundingRect() const override;

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) override;

    int row_height() const;

    int row_count() const;

    int visible_height() const;

    void add_row(LineItem* row, int index);

    void remove_rows(int first, int last);

    void move_row(int from, int to);

    void expand();

    void collapse();

    void set_expanded(bool expanded);

    bool is_expanded();

    LineItem* parent_line() const;

    int visible_rows() const;

    void raw_clear();

    virtual item_models::PropertyModelFull::Item property_item() const
    {
        return {};
    }

    const std::vector<LineItem*>& rows() const
    {
        return rows_;
    }

    int type() const override { return int(ItemTypes::LineItem); }

    quintptr id() const { return id_; }

signals:
    void removed(quintptr id, QPrivateSignal = {});
    void clicked(quintptr id, bool selected, bool replace_selection);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent * event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent * event) override;

    virtual void on_set_time_start(int){}
    virtual void on_set_time_end(int){}

    void click_selected(bool selected, bool replace_selection);

private:
    void propagate_row_vis(int delta);

    void adjust_row_vis(int delta, bool propagate = true);

    void emit_removed();

    int time_start;
    int time_end;
    int height_;
    model::Object* object_;
    std::vector<LineItem*> rows_;
    int visible_rows_ = 1;
    quintptr id_ = 0;
    bool expanded_ = false;
};

class ObjectLineItem : public LineItem
{
    Q_OBJECT

public:
    ObjectLineItem(quintptr id, model::Object* obj, int time_start, int time_end, int height)
        : LineItem(id, obj, time_start, time_end, height)
    {}

    int type() const override { return int(ItemTypes::ObjectLineItem); }

    item_models::PropertyModelFull::Item property_item() const override
    {
        return {object(), nullptr};
    }
};

class AnimatableItem;

class KeyframeSplitItem : public QGraphicsObject
{
    Q_OBJECT

public:
    static constexpr const int icon_size = 16;
    static constexpr const int pen = 2;
    static constexpr const QSize half_icon_size{icon_size/2, icon_size};

    KeyframeSplitItem(AnimatableItem* parent);

    QRectF boundingRect() const override
    {
        return QRectF(-icon_size/2-pen, -icon_size/2-pen, icon_size+2*pen, icon_size+2*pen);
    }

    void paint(QPainter * painter, const QStyleOptionGraphicsItem *, QWidget * widget) override;


    void set_enter(model::KeyframeTransition::Descriptive enter);

    void set_exit(model::KeyframeTransition::Descriptive exit);

    model::FrameTime time() const { return x(); }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent * event) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent * event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent * event) override;

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

    bool drag_allowed() const
    {
        /// \todo Handle for stuff like transform attributes :/
        return !visual_node || !visual_node->docnode_locked_recursive();
    }

    void drag_init()
    {
        drag_start = x();
        dragging = drag_allowed();
    }

    void drag_move(qreal delta)
    {
        if ( dragging )
            setX(drag_start+delta);
    }

    void drag_end()
    {
        dragging = false;
    }

    AnimatableItem* line() const;

    QPixmap pix_enter;
    QPixmap pix_exit;
    QIcon icon_enter;
    QIcon icon_exit;
    model::FrameTime drag_start;
    bool dragging = false;
    model::VisualNode* visual_node = nullptr;
};

class AnimatableItem : public LineItem
{
    Q_OBJECT

private:
    struct DragData
    {
        KeyframeSplitItem* item;
        model::FrameTime from;
        model::FrameTime to;
    };

public:
    AnimatableItem(quintptr id, model::Object* obj, model::AnimatableBase* animatable, int time_start, int time_end, int height);

    std::pair<model::KeyframeBase*, model::KeyframeBase*> keyframes(KeyframeSplitItem* item);

    int type() const override;

    item_models::PropertyModelFull::Item property_item() const override;

public slots:
    void add_keyframe(int index);

    void remove_keyframe(int index);

private slots:
    void transition_changed(model::KeyframeTransition::Descriptive before, model::KeyframeTransition::Descriptive after);


    void update_keyframe(int index, model::KeyframeBase* kf);

private:
    void keyframes_dragged(const std::vector<DragData>& keyframe_items);

    model::AnimatableBase* animatable;
    std::vector<KeyframeSplitItem*> kf_split_items;
    friend KeyframeSplitItem;
};

class TimeRectItem : public QGraphicsObject
{
public:
    TimeRectItem(model::VisualNode* node, qreal height, QGraphicsItem* parent)
    : QGraphicsObject(parent),
      radius(height/2)
    {
        update_color(node->docnode_group_color());
        connect(node, &model::VisualNode::docnode_group_color_changed, this, &TimeRectItem::update_color);
    }

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * opt, QWidget *) override
    {
        QColor fill = math::lerp(opt->palette.base().color(), color, color.alphaF());
        fill.setAlpha(255);
        auto stroke = fill.lightnessF() < 0.4 ? Qt::white : Qt::black;
        QPen p(stroke, 1);
        p.setCosmetic(true);
        painter->setPen(p);
        painter->setBrush(fill);
        painter->drawRect(boundingRect());
    }

private:
    void update_color(const QColor& col)
    {
        color = col;
        update();
    }

protected:
    QColor color;
    qreal radius;
};

class AnimationContainerItem : public TimeRectItem
{
public:
    AnimationContainerItem(model::VisualNode* node, model::AnimationContainer* animation,
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
    PropertyLineItem(quintptr id, model::Object* obj, model::BaseProperty* prop, int time_start, int time_end, int height)
        : LineItem(id, obj, time_start, time_end, height),
        property_(prop)
    {}

    int type() const override { return int(ItemTypes::PropertyLineItem); }

    model::BaseProperty* property() const
    {
        return property_;
    }

    item_models::PropertyModelFull::Item property_item() const override
    {
        return {object(), property_};
    }

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

class ObjectListLineItem : public LineItem
{
    Q_OBJECT

public:
    ObjectListLineItem(quintptr id, model::Object* obj, model::ObjectListPropertyBase* prop, int time_start, int time_end, int height)
        : LineItem(id, obj, time_start, time_end, height),
        property_(prop)
    {}

    int type() const override { return int(ItemTypes::ObjectListLineItem); }

    model::BaseProperty* property() const
    {
        return property_;
    }

    item_models::PropertyModelFull::Item property_item() const override
    {
        return {object(), property_};
    }

private:
    model::ObjectListPropertyBase* property_;
};


} // namespace glaxnimate::gui::timeline
