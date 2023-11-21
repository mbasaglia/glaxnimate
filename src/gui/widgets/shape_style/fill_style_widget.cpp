/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "fill_style_widget.hpp"

#include "model/assets/named_color.hpp"
#include "model/document.hpp"
#include "command/animation_commands.hpp"

using namespace glaxnimate::gui;
using namespace glaxnimate;


FillStyleWidget::FillStyleWidget(QWidget* parent )
: ColorSelector(parent)
{
    connect(this, &ColorSelector::current_color_changed,
            this, &FillStyleWidget::set_target_color);
    connect(this, &ColorSelector::current_color_committed,
            this, &FillStyleWidget::commit_target_color);
    connect(this, &ColorSelector::current_color_cleared,
            this, &FillStyleWidget::clear_target_color);
}


void FillStyleWidget::set_targets(const std::vector<model::Fill*>& new_targets)
{
    targets.clear();
    targets.assign(new_targets.begin(), new_targets.end());
}

void glaxnimate::gui::FillStyleWidget::before_set_target()
{
    if ( current_target )
    {
        disconnect(current_target, &model::Object::property_changed,
                    this, &FillStyleWidget::property_changed);
    }
}

void glaxnimate::gui::FillStyleWidget::after_set_target()
{
    if ( current_target )
    {
        update_from_target();
        connect(current_target, &model::Object::property_changed,
                this, &FillStyleWidget::property_changed);
    }
}

void glaxnimate::gui::FillStyleWidget::set_current(model::Fill* current)
{
    before_set_target();

    current_target = current;
    stop = -1;

    after_set_target();

}

model::Fill * FillStyleWidget::current() const
{
    return static_cast<model::Fill*>(current_target);
}

void FillStyleWidget::update_from_target()
{
    auto lock = updating.get_lock();
    from_styler(current_target, stop);
    Q_EMIT current_color_changed(current_color());
    update();
}

void FillStyleWidget::set_target_color(const QColor& color)
{
    set_color(color, false);
}

void FillStyleWidget::commit_target_color()
{
    if ( current_target )
        set_color(current_target->color.get(), true);
}

void FillStyleWidget::property_changed(const model::BaseProperty* prop)
{
    if ( prop == &current_target->color || prop == &current_target->use )
    {
        update_from_target();
    }
}

void FillStyleWidget::set_color(const QColor&, bool commit)
{
    if ( updating )
        return;

    apply_to_targets(tr("Update Fill Color"), targets, stop, commit);
}

void FillStyleWidget::set_gradient_stop(model::Styler* styler, int index)
{
    if ( auto fill = styler->cast<model::Fill>() )
    {
        before_set_target();
        current_target = fill;
        targets.push_back(fill);
        targets.clear();
        stop = index;
        after_set_target();
    }
}

void FillStyleWidget::clear_target_color()
{
    clear_targets(tr("Clear Fill Color"), targets);
}
