#pragma once

#include<set>

#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include "handle.hpp"
#include "model/shapes/path.hpp"
#include "utils/pseudo_mutex.hpp"

namespace graphics {

class BezierPointItem : public QGraphicsObject
{
    Q_OBJECT
public:
    BezierPointItem(int index, const math::BezierPoint& point, QGraphicsItem* parent);

    QRectF boundingRect() const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *) override;

    int index() const;

    void set_point_type(math::BezierPointType type);

    void show_tan_in(bool show);

    void show_tan_out(bool show);

    void remove_tangent(MoveHandle* handle);

    const math::BezierPoint& point() const;
    void modify(const math::BezierPoint& pt, const QString& undo_name);

    class BezierItem* parent_editor() const;

    bool tan_in_empty() const;
    bool tan_out_empty() const;

signals:
    void modified(int index, const math::BezierPoint& point, bool commit, const QString& name);

private slots:
    void tan_in_dragged(const QPointF& p, Qt::KeyboardModifiers mods);

    void tan_out_dragged(const QPointF& p, Qt::KeyboardModifiers mods);

    void pos_dragged(const QPointF& p);

    void on_modified(bool commit, const QString& name="");
    void on_commit();

private:
    void drag_preserve_angle(QPointF& dragged, QPointF& other, const QPointF& dragged_new);

    void set_point(const math::BezierPoint& p);
    void set_index(int index);
    void set_has_tan_in(bool show);
    void set_has_tan_out(bool show);


    MoveHandle pos{nullptr, MoveHandle::Any, MoveHandle::Diamond, 8};
    MoveHandle tan_in{nullptr, MoveHandle::Any, MoveHandle::Circle, 6};
    MoveHandle tan_out{nullptr, MoveHandle::Any, MoveHandle::Circle, 6};

    int index_;
    math::BezierPoint point_;
    bool has_tan_in = true;
    bool has_tan_out = true;
    friend class BezierItem;
};


class BezierItem : public QGraphicsObject
{
    Q_OBJECT

public:
    BezierItem(model::Path* node, QGraphicsItem* parent=nullptr);

    QRectF boundingRect() const override;

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget*) override;

    void set_type(int index, math::BezierPointType type);

    model::AnimatedProperty<math::Bezier>* target_property() const;
    model::DocumentNode* target_object() const;

    const std::set<int>& selected_indices();
    void clear_selected_indices();
    void select_index(int i);
    void deselect_index(int i);
    void toggle_index(int i);


public slots:
    void set_bezier(const math::Bezier& bez);

    void remove_point(int index);

private slots:
    void on_dragged(int index, const math::BezierPoint& point, bool commit, const QString& name);

private:
    void do_update(bool commit, const QString& name);
    void do_add_point(int index);

    math::Bezier bezier_;
    std::vector<std::unique_ptr<BezierPointItem>> items;
    model::Path* node;
    utils::PseudoMutex updating;
    std::set<int> selected_indices_;
};


} // namespace graphics
