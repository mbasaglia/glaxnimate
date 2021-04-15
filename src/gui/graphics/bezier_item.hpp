#pragma once

#include <set>

#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include "handle.hpp"
#include "model/shapes/path.hpp"
#include "utils/pseudo_mutex.hpp"
#include "typed_item.hpp"

namespace graphics {

class PointItem : public QGraphicsObject
{
    Q_OBJECT
public:
    PointItem(int index, const math::bezier::Point& point, QGraphicsItem* parent, model::AnimatedProperty<math::bezier::Bezier>* property);

    QRectF boundingRect() const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *) override;

    int index() const;

    void set_point_type(math::bezier::PointType type);

    void show_tan_in(bool show);

    void show_tan_out(bool show);

    void remove_tangent(MoveHandle* handle);

    const math::bezier::Point& point() const;
    void modify(const math::bezier::Point& pt, const QString& undo_name);

    class BezierItem* parent_editor() const;

    bool tan_in_empty() const;
    bool tan_out_empty() const;

    void set_selected(bool selected);

signals:
    void modified(int index, const math::bezier::Point& point, bool commit, const QString& name);

private slots:
    void tan_in_dragged(const QPointF& p, Qt::KeyboardModifiers mods);

    void tan_out_dragged(const QPointF& p, Qt::KeyboardModifiers mods);

    void pos_dragged(const QPointF& p);

    void on_modified(bool commit, const QString& name="");
    void on_commit();

private:
    void drag_preserve_angle(QPointF& dragged, QPointF& other, const QPointF& dragged_new);

    void set_point(const math::bezier::Point& p);
    void set_index(int index);
    void set_has_tan_in(bool show);
    void set_has_tan_out(bool show);


    MoveHandle pos{nullptr, MoveHandle::Any, MoveHandle::Diamond, 8};
    MoveHandle tan_in{nullptr, MoveHandle::Any, MoveHandle::Circle, 6};
    MoveHandle tan_out{nullptr, MoveHandle::Any, MoveHandle::Circle, 6};

    int index_;
    math::bezier::Point point_;
    bool has_tan_in = true;
    bool has_tan_out = true;
    bool shows_tan_in = true;
    bool shows_tan_out = true;
    friend class BezierItem;
};


class BezierItem : public TypedItem<Types::BezierItem>
{
    Q_OBJECT

public:
    BezierItem(model::Path* node, QGraphicsItem* parent=nullptr);

    QRectF boundingRect() const override;

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget*) override;

    void set_type(int index, math::bezier::PointType type);

    model::AnimatedProperty<math::bezier::Bezier>* target_property() const;
    model::VisualNode* target_object() const;

    const std::set<int>& selected_indices();
    void clear_selected_indices();
    void select_index(int i);
    void deselect_index(int i);
    void toggle_index(int i);

    const math::bezier::Bezier& bezier() const;

public slots:
    void set_bezier(const math::bezier::Bezier& bez);

    void remove_point(int index);

private slots:
    void on_dragged(int index, const math::bezier::Point& point, bool commit, const QString& name);

private:
    void do_update(bool commit, const QString& name);
    void do_add_point(int index);

    math::bezier::Bezier bezier_;
    std::vector<std::unique_ptr<PointItem>> items;
    model::Path* node;
    utils::PseudoMutex updating;
    std::set<int> selected_indices_;
};


} // namespace graphics
