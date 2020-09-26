#include "fill_style_widget.hpp"

#include "model/defs/named_color.hpp"
#include "model/document.hpp"
#include "command/animation_commands.hpp"


FillStyleWidget::FillStyleWidget(QWidget* parent )
: ColorSelector(parent)
{
    connect(this, &ColorSelector::current_color_changed,
            this, &FillStyleWidget::set_target_color);
    connect(this, &ColorSelector::current_color_committed,
            this, &FillStyleWidget::commit_target_color);
    connect(this, &ColorSelector::current_color_def,
            this, &FillStyleWidget::set_target_def);
}


void FillStyleWidget::set_shape(model::Fill* target)
{
    if ( this->target )
    {
        disconnect(this->target, &model::Object::property_changed,
                    this, &FillStyleWidget::property_changed);
    }

    this->target = target;

    if ( target )
    {
        update_from_target();
        connect(target, &model::Object::property_changed,
                this, &FillStyleWidget::property_changed);
    }
}

model::Fill * FillStyleWidget::shape() const
{
    return target;
}


void FillStyleWidget::update_from_target()
{
    updating = true;
    QColor color;
    if (  auto named_color = qobject_cast<model::NamedColor*>(target->use.get()) )
        color = named_color->color.get();
    else
        color = target->color.get();
    set_current_color(color);
    emit current_color_changed(color);
    update();
    updating = false;
}

void FillStyleWidget::set_target_color(const QColor& color)
{
    set_color(color, false);
}

void FillStyleWidget::commit_target_color()
{
    if ( target )
        set_color(target->color.get(), true);
}

void FillStyleWidget::property_changed(const model::BaseProperty* prop)
{
    if ( prop == &target->color || prop == &target->use )
    {
        update_from_target();
    }
}

void FillStyleWidget::set_target_def(model::BrushStyle* def)
{
    if ( !target || updating )
        return;

    if ( !def )
    {
        target->document()->undo_stack().beginMacro(tr("Unlink Fill Color"));
        if ( auto col = qobject_cast<model::NamedColor*>(target->use.get()) )
            target->color.set_undoable(col->color.get());
        target->use.set_undoable(QVariant::fromValue(def));
        target->document()->undo_stack().endMacro();
    }
    else
    {
        target->document()->undo_stack().beginMacro(tr("Link Fill Color"));
        target->use.set_undoable(QVariant::fromValue(def));
        target->document()->undo_stack().endMacro();
    }
}

void FillStyleWidget::set_color(const QColor& color, bool commit)
{
    if ( !target || updating )
        return;

    if (  auto named_color = qobject_cast<model::NamedColor*>(target->use.get()) )
    {
        target->push_command(new command::SetMultipleAnimated(
            tr("Update Fill Color"),
            commit,
            {&named_color->color, &target->color},
            color,
            color
        ));
    }
    else
    {
        target->color.set_undoable(color, commit);
    }
}
