#pragma once

#include <QGraphicsObject>
#include <list>

#include "glaxnimate/core/model/shapes/styler.hpp"
#include "glaxnimate/core/model/assets/gradient.hpp"

#include "handle.hpp"

namespace glaxnimate::gui::graphics {

class GradientEditor : public QGraphicsObject
{
public:
    GradientEditor(model::Styler* styler);

    QRectF boundingRect() const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *) override;

    void remove_highlight();
    void show_highlight();
    bool highlight_visible() const;

    model::Styler* styler() const;
    model::Gradient* gradient() const;

private:
    void on_use_changed(model::BrushStyle* new_use);

    void start_dragged(QPointF p, Qt::KeyboardModifiers mods);
    void start_committed();
    void finish_dragged(QPointF p, Qt::KeyboardModifiers mods);
    void finish_committed();
    void highlight_dragged(const QPointF& p);
    void highlight_committed();
    QString command_name() const;

    void update_stops();
    void update_stop_pos();
    void stop_dragged();
    void stop_committed();
    void stop_move(bool commit);
    void update_stops_from_gradient();

    MoveHandle start{this, MoveHandle::Any, MoveHandle::Square};
    MoveHandle finish{this, MoveHandle::Any, MoveHandle::Diamond, 8};
    MoveHandle highlight{this, MoveHandle::Any, MoveHandle::Saltire, 8};
    std::list<MoveHandle> stops;

    model::Styler* styler_;
    model::Gradient* gradient_ = nullptr;
};

} // namespace glaxnimate::gui::graphics
