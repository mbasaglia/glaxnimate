#include "gradient_editor.hpp"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include "math/geom.hpp"
#include "command/animation_commands.hpp"

graphics::GradientEditor::GradientEditor(model::Styler* styler)
    : styler(styler)
{
    start.setVisible(false);
    finish.setVisible(false);
    highlight.setVisible(false);
    on_use_changed(styler->use.get());

    connect(styler, &model::Styler::use_changed, this, &GradientEditor::on_use_changed);

    connect(&start,     &MoveHandle::dragged,       this, &GradientEditor::start_dragged);
    connect(&start,     &MoveHandle::drag_finished, this, &GradientEditor::start_committed);
    connect(&finish,    &MoveHandle::dragged,       this, &GradientEditor::finish_dragged);
    connect(&finish,    &MoveHandle::drag_finished, this, &GradientEditor::finish_committed);
    connect(&highlight, &MoveHandle::dragged,       this, &GradientEditor::highlight_dragged);
    connect(&highlight, &MoveHandle::drag_finished, this, &GradientEditor::highlight_committed);
}


void graphics::GradientEditor::on_use_changed(model::BrushStyle* new_use)
{
    if ( new_use == gradient )
        return;

    gradient = new_use->cast<model::Gradient>();

    if ( gradient && !gradient->colors.get() )
        gradient = nullptr;

    if ( !gradient )
    {
        start.setVisible(false);
        finish.setVisible(false);
        highlight.setVisible(false);
        return;
    }

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

    styler->push_command(cmd);
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

    styler->push_command(cmd);
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

    styler->push_command(
        new command::SetMultipleAnimated(&gradient->end_point, p, false)
    );
    update();
}

void graphics::GradientEditor::finish_committed()
{
    if ( !gradient )
        return;

    styler->push_command(
        new command::SetMultipleAnimated(&gradient->end_point, gradient->end_point.value(), true)
    );
}


void graphics::GradientEditor::highlight_dragged(const QPointF& p)
{
    if ( !gradient )
        return;

    styler->push_command(
        new command::SetMultipleAnimated(&gradient->highlight, p, false)
    );
}

void graphics::GradientEditor::highlight_committed()
{
    if ( !gradient )
        return;

    styler->push_command(
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
    QPen p(option->palette.highlight(), 1);
    p.setCosmetic(true);
    painter->setPen(p);
    painter->drawLine(start.pos(), finish.pos());
}
