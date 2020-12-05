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


    if ( new_gradient == gradient_ )
        return;

    if ( gradient_ )
        disconnect(gradient_, &model::Gradient::style_changed, this, &GradientEditor::update_stops_from_gradient);

    gradient_ = new_gradient;

    update_stops();

    if ( !gradient_ )
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


    start.set_associated_properties({&gradient_->start_point, &gradient_->colors->colors});
    finish.set_associated_properties({&gradient_->end_point, &gradient_->colors->colors});
    highlight.set_associated_properties({&gradient_->highlight, &gradient_->colors->colors});

    connect(gradient_, &model::Gradient::style_changed, this, &GradientEditor::update_stops_from_gradient);

    start.setVisible(true);
    finish.setVisible(true);

    if ( gradient_->type.get() == model::Gradient::Radial && gradient_->highlight.get() != gradient_->start_point.get() )
        highlight.setVisible(true);
    else
        highlight.setVisible(false);

    start.setPos(gradient_->start_point.get());
    finish.setPos(gradient_->end_point.get());
    highlight.setPos(gradient_->highlight.get());
    update();
}

QString graphics::GradientEditor::command_name() const
{
    return tr("Drag Gradient");
}

void graphics::GradientEditor::start_dragged(QPointF p, Qt::KeyboardModifiers mods)
{
    if ( !gradient_ )
        return;

    if ( mods & Qt::ControlModifier )
    {
        p = math::line_closest_point(gradient_->start_point.get(), gradient_->end_point.get(), p);
        start.setPos(p);
    }

    auto cmd = new command::SetMultipleAnimated(command_name(), false);
    cmd->push_property(&gradient_->start_point, p);

    if ( !highlight.isVisible() )
        cmd->push_property(&gradient_->highlight, p);

    styler_->push_command(cmd);

    update_stop_pos();
    update();
}


void graphics::GradientEditor::start_committed()
{
    if ( !gradient_ )
        return;

    auto cmd = new command::SetMultipleAnimated(command_name(), true);
    cmd->push_property(&gradient_->start_point, gradient_->start_point.value());

    if ( !highlight.isVisible() )
        cmd->push_property(&gradient_->highlight, gradient_->start_point.value());

    styler_->push_command(cmd);
}

void graphics::GradientEditor::finish_dragged(QPointF p, Qt::KeyboardModifiers mods)
{
    if ( !gradient_ )
        return;

    if ( mods & Qt::ControlModifier )
    {
        p = math::line_closest_point(gradient_->start_point.get(), gradient_->end_point.get(), p);
        finish.setPos(p);
    }


    styler_->push_command(
        new command::SetMultipleAnimated(&gradient_->end_point, p, false)
    );

    update_stop_pos();
    update();
}

void graphics::GradientEditor::finish_committed()
{
    if ( !gradient_ )
        return;

    styler_->push_command(
        new command::SetMultipleAnimated(&gradient_->end_point, gradient_->end_point.value(), true)
    );
}


void graphics::GradientEditor::highlight_dragged(const QPointF& p)
{
    if ( !gradient_ )
        return;

    styler_->push_command(
        new command::SetMultipleAnimated(&gradient_->highlight, p, false)
    );
}

void graphics::GradientEditor::highlight_committed()
{
    if ( !gradient_ )
        return;

    styler_->push_command(
        new command::SetMultipleAnimated(&gradient_->highlight, gradient_->highlight.value(), true)
    );
}

QRectF graphics::GradientEditor::boundingRect() const
{
    if ( !gradient_ )
        return {};

    return QRectF(start.pos(), finish.pos());
}

void graphics::GradientEditor::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*)
{
    if ( gradient_ )
    {
        QPen p(option->palette.highlight(), 1);
        p.setCosmetic(true);
        painter->setPen(p);
        painter->drawLine(start.pos(), finish.pos());
    }
}

void graphics::GradientEditor::remove_highlight()
{
    if ( gradient_ && gradient_->type.get() == model::Gradient::Radial )
    {
        highlight.setPos(start.pos());
        highlight.setVisible(false);
        gradient_->highlight.set_undoable(gradient_->start_point.get());
    }
}

void graphics::GradientEditor::show_highlight()
{
    if ( gradient_ && gradient_->type.get() == model::Gradient::Radial )
    {
        highlight.setVisible(true);
        highlight.setPos(gradient_->highlight.get());
    }
}

void graphics::GradientEditor::update_stops()
{
    stops.clear();
    if ( !gradient_ )
        return;

    QPointF start = gradient_->start_point.get();
    QPointF end = gradient_->end_point.get();

    this->start.setPos(start);
    finish.setPos(end);
    highlight.setPos(gradient_->highlight.get());

    int i = 0;
    for ( const auto& stop : gradient_->colors->colors.get() )
    {
        stops.emplace_back(this, MoveHandle::Any, MoveHandle::Diamond);
        stops.back().set_role(MoveHandle::GradientStop);
        stops.back().setData(graphics::GradientStopIndex, i++);
        stops.back().setPos(math::lerp(start, end, stop.first));
        stops.back().setZValue(-10);
        stops.back().set_associated_property(&gradient_->colors->colors);
        connect(&stops.back(), &MoveHandle::dragged, this, &GradientEditor::stop_dragged);
        connect(&stops.back(), &MoveHandle::drag_finished, this, &GradientEditor::stop_committed);
    }
    finish.setData(graphics::GradientStopIndex, gradient_->colors->colors.get().size() - 1);

    update();
}

void graphics::GradientEditor::update_stop_pos()
{
    QPointF start = gradient_->start_point.get();
    QPointF end = gradient_->end_point.get();

    this->start.setPos(start);
    finish.setPos(end);
    highlight.setPos(gradient_->highlight.get());

    const auto& colors = gradient_->colors->colors.get();
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

    auto colors = gradient_->colors->colors.get();
    if ( index < 0 || index >= colors.size() )
        return;

    QPointF start = gradient_->start_point.get();
    QPointF end = gradient_->end_point.get();
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

    gradient_->push_command(new command::SetMultipleAnimated(
        &gradient_->colors->colors,
        QVariant::fromValue(colors),
        commit
    ));

    if ( commit )
        update_stop_pos();
}

void graphics::GradientEditor::update_stops_from_gradient()
{
    if ( !gradient_->colors.get() )
    {
        on_use_changed(nullptr);
        update_stops();
        return;
    }

    const auto& colors = gradient_->colors->colors.get();

    if ( colors.size() != int(stops.size()) )
        update_stops();
    else
        update_stop_pos();
}

model::Styler * graphics::GradientEditor::styler() const
{
    return styler_;
}

model::Gradient * graphics::GradientEditor::gradient() const
{
    return gradient_;
}

bool graphics::GradientEditor::highlight_visible() const
{
    return highlight.isVisible();
}

