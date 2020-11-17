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
}


void FillStyleWidget::set_shape(model::Fill* target, int gradient_stop)
{
    if ( this->target )
    {
        disconnect(this->target, &model::Object::property_changed,
                    this, &FillStyleWidget::property_changed);
    }

    this->target = target;
    stop = gradient_stop;

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
    auto lock = updating.get_lock();
    from_styler(target, stop);
    emit current_color_changed(current_color());
    update();
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

void FillStyleWidget::set_color(const QColor&, bool commit)
{
    if ( updating )
        return;

    to_styler(tr("Update Fill Color"), target, stop, commit);
}

void FillStyleWidget::set_gradient_stop(model::Styler* styler, int index)
{
    if ( auto fill = styler->cast<model::Fill>() )
        set_shape(fill, index);
}
