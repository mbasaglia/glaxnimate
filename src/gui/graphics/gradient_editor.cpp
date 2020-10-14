#include "gradient_editor.hpp"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include "math/geom.hpp"
#include "command/animation_commands.hpp"
#include "item_data.hpp"
#include "utils/sort_gradient.hpp"

graphics::GradientEditor::GradientEditor(model::Styler* styler)
    : styler_(styler)
{
    start.setVisible(false);
    finish.setVisible(false);
    highlight.setVisible(false);

    start.set_role(MoveHandle::GradientStop);
    finish.set_role(MoveHandle::GradientStop);
    highlight.set_role(MoveHandle::GradientHighlight);

    start.setData(graphics::GradientStopIndex, 0);
    finish.setData(graphics::GradientStopIndex, 0);

    on_use_changed(styler_->use.get());

    connect(styler_, &model::Styler::use_changed, this, &GradientEditor::on_use_changed);

    connect(&start,     &MoveHandle::dragged,       this, &GradientEditor::start_dragged);
    connect(&start,     &MoveHandle::drag_finished, this, &GradientEditor::start_committed);
    connect(&finish,    &MoveHandle::dragged,       this, &GradientEditor::finish_dragged);
    connect(&finish,    &MoveHandle::drag_finished, this, &GradientEditor::finish_committed);
    connect(&highlight, &MoveHandle::dragged,       this, &GradientEditor::highlight_dragged);
    connect(&highlight, &MoveHandle::drag_finished, this, &GradientEditor::highlight_committed);
    connect(&start, &MoveHandle::clicked, this, [this](Qt::KeyboardModifiers mod){
        if ( (mod & Qt::ShiftModifier) )
            show_highlight();
    });
    connect(&highlight, &MoveHandle::clicked, this, [this](Qt::KeyboardModifiers mod){
        if ( (mod & Qt::ShiftModifier) )
            remove_highlight();
    });
}

void graphics::GradientEditor::on_use_changed(model::BrushStyle* new_use)
{
    auto new_gradient = new_use->cast<model::Gradient>();

    if ( new_gradient && !new_gradient->colors.get() )
        new_gradient = nullptr;


    if ( new_gradient == gradient )
        return;

    if ( gradient )
        disconnect(gradient, &model::Gradient::style_changed, this, &GradientEditor::update_stops_from_gradient);

    gradient = new_gradient;

    update_stops();

    if ( !gradient )
    {
        start.setVisible(false);
        finish.setVisible(false);
        highlight.setVisible(false);

        start.clear_associated_properties();
        finish.clear_associated_properties();
        highlight.clear_associated_properties();
        update();
        return;
    }


    start.set_associated_property(&gradient->start_point);
    finish.set_associated_property(&gradient->end_point);
    highlight.set_associated_property(&gradient->highlight);

    connect(gradient, &model::Gradient::style_changed, this, &GradientEditor::update_stops_from_gradient);

    start.setVisible(true);
    finish.setVisible(true);

    if ( gradient->type.get() == model::Gradient::Radial && gradient->highlight.get() != gradient->start_point.get() )
        highlight.setVisible(true);
    else
        highlight.setVisible(false);

    start.setPos(gradient->start_point.get());
    finish.setPos(gradient->end_point.get());
    highlight.setPos(gradient->highlight.get());
    update();
}

QString graphics::GradientEditor::command_name() const
{
    return tr("Drag Gradient");
}

void graphics::GradientEditor::start_dragged(QPointF p, Qt::KeyboardModifiers mods)
{
    if ( !gradient )
        return;

    if ( mods & Qt::ControlModifier )
    {
        p = math::line_closest_point(gradient->start_point.get(), gradient->end_point.get(), p);
        start.setPos(p);
    }

    auto cmd = new command::SetMultipleAnimated(command_name(), false);
    cmd->push_property(&gradient->start_point, p);

    if ( !highlight.isVisible() )
        cmd->push_property(&gradient->highlight, p);

    styler_->push_command(cmd);

    update_stop_pos();
    update();
}


void graphics::GradientEditor::start_committed()
{
    if ( !gradient )
        return;

    auto cmd = new command::SetMultipleAnimated(command_name(), true);
    cmd->push_property(&gradient->start_point, gradient->start_point.value());

    if ( !highlight.isVisible() )
        cmd->push_property(&gradient->highlight, gradient->start_point.value());

    styler_->push_command(cmd);
}

void graphics::GradientEditor::finish_dragged(QPointF p, Qt::KeyboardModifiers mods)
{
    if ( !gradient )
        return;

    if ( mods & Qt::ControlModifier )
    {
        p = math::line_closest_point(gradient->start_point.get(), gradient->end_point.get(), p);
        finish.setPos(p);
    }


    styler_->push_command(
        new command::SetMultipleAnimated(&gradient->end_point, p, false)
    );

    update_stop_pos();
    update();
}

void graphics::GradientEditor::finish_committed()
{
    if ( !gradient )
        return;

    styler_->push_command(
        new command::SetMultipleAnimated(&gradient->end_point, gradient->end_point.value(), true)
    );
}


void graphics::GradientEditor::highlight_dragged(const QPointF& p)
{
    if ( !gradient )
        return;

    styler_->push_command(
        new command::SetMultipleAnimated(&gradient->highlight, p, false)
    );
}

void graphics::GradientEditor::highlight_committed()
{
    if ( !gradient )
        return;

    styler_->push_command(
        new command::SetMultipleAnimated(&gradient->highlight, gradient->highlight.value(), true)
    );
}

QRectF graphics::GradientEditor::boundingRect() const
{
    if ( !gradient )
        return {};

    return QRectF(start.pos(), finish.pos());
}

void graphics::GradientEditor::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*)
{
    if ( gradient )
    {
        QPen p(option->palette.highlight(), 1);
        p.setCosmetic(true);
        painter->setPen(p);
        painter->drawLine(start.pos(), finish.pos());
    }
}

void graphics::GradientEditor::remove_highlight()
{
    if ( gradient && gradient->type.get() == model::Gradient::Radial )
    {
        highlight.setPos(start.pos());
        highlight.setVisible(false);
        gradient->highlight.set_undoable(gradient->start_point.get());
    }
}

void graphics::GradientEditor::show_highlight()
{
    if ( gradient && gradient->type.get() == model::Gradient::Radial )
    {
        highlight.setVisible(true);
        highlight.setPos(gradient->highlight.get());
    }
}

void graphics::GradientEditor::update_stops()
{
    stops.clear();
    if ( !gradient )
        return;

    QPointF start = gradient->start_point.get();
    QPointF end = gradient->end_point.get();

    this->start.setPos(start);
    finish.setPos(end);
    highlight.setPos(gradient->highlight.get());

    int i = 0;
    for ( const auto& stop : gradient->colors->colors.get() )
    {
        stops.emplace_back(this, MoveHandle::Any, MoveHandle::Diamond);
        stops.back().set_role(MoveHandle::GradientStop);
        stops.back().setData(graphics::GradientStopIndex, i++);
        stops.back().setPos(math::lerp(start, end, stop.first));
        stops.back().setZValue(-10);
        stops.back().set_associated_property(&gradient->colors->colors);
        connect(&stops.back(), &MoveHandle::dragged, this, &GradientEditor::stop_dragged);
        connect(&stops.back(), &MoveHandle::drag_finished, this, &GradientEditor::stop_committed);
    }
    finish.setData(graphics::GradientStopIndex, gradient->colors->colors.get().size() - 1);

    update();
}

void graphics::GradientEditor::update_stop_pos()
{
    QPointF start = gradient->start_point.get();
    QPointF end = gradient->end_point.get();

    this->start.setPos(start);
    finish.setPos(end);
    highlight.setPos(gradient->highlight.get());

    const auto& colors = gradient->colors->colors.get();
    int i = 0;
    for ( auto& handle : stops )
    {
        handle.setPos(math::lerp(start, end, colors[i++].first));
    }

    update();
}

void graphics::GradientEditor::stop_dragged()
{
    stop_move(false);
}

void graphics::GradientEditor::stop_committed()
{
    stop_move(true);
}

void graphics::GradientEditor::stop_move(bool commit)
{
    auto handle = static_cast<MoveHandle*>(sender());
    int index = handle->data(graphics::GradientStopIndex).toInt();

    auto colors = gradient->colors->colors.get();
    if ( index < 0 || index >= colors.size() )
        return;

    QPointF start = gradient->start_point.get();
    QPointF end = gradient->end_point.get();
    QPointF pos = math::line_closest_point(start, end, handle->pos());
    qreal ratio = 0;

    if ( start == end )
    {
        pos = start;
    }
    else
    {
        // reverse interpolation on one component
        qreal a, b, p;
        if ( start.x() == end.x() )
        {
            a = start.y();
            b = end.y();
            p = pos.y();
        }
        else
        {
            a = start.x();
            b = end.x();
            p = pos.x();
        }

        // derived from p = a*(1-ratio) + b*ratio
        ratio = (p-a) / (b-a);

        if ( ratio < 0 )
        {
            ratio = 0;
            pos = start;
        }
        else if ( ratio > 1 )
        {
            ratio = 1;
            pos = end;
        }
    }

    handle->setPos(pos);
    colors[index].first = ratio;

    if ( commit )
        utils::sort_gradient(colors);

    gradient->push_command(new command::SetMultipleAnimated(
        &gradient->colors->colors,
        QVariant::fromValue(colors),
        commit
    ));

    if ( commit )
        update_stop_pos();
}

void graphics::GradientEditor::update_stops_from_gradient()
{
    if ( !gradient->colors.get() )
        on_use_changed(nullptr);

    const auto& colors = gradient->colors->colors.get();

    if ( colors.size() != int(stops.size()) )
        update_stops();
    else
        update_stop_pos();
}

model::Styler * graphics::GradientEditor::styler()
{
    return styler_;
}
