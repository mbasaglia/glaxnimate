/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "styler.hpp"
#include "model/document.hpp"
#include "model/assets/named_color.hpp"

std::vector<glaxnimate::model::DocumentNode*> glaxnimate::model::Styler::valid_uses() const
{
    auto v = document()->assets()->gradients->values.valid_reference_values(true);
    auto v2 = document()->assets()->colors->values.valid_reference_values(false);
    v.insert(v.end(), v2.begin(), v2.end());
    return v;
}

bool glaxnimate::model::Styler::is_valid_use(DocumentNode* node) const
{
    return
        document()->assets()->gradients->values.is_valid_reference_value(node, true) ||
        document()->assets()->colors->values.is_valid_reference_value(node, false);
}

void glaxnimate::model::Styler::on_use_changed(glaxnimate::model::BrushStyle* new_use, glaxnimate::model::BrushStyle* old_use)
{
    QColor reset;

    if ( old_use )
    {
        disconnect(old_use, &BrushStyle::style_changed, this, &Styler::on_update_style);
        if ( auto old_col = qobject_cast<glaxnimate::model::NamedColor*>(old_use) )
            reset = old_col->color.get();
    }

    if ( new_use )
    {
        connect(new_use, &BrushStyle::style_changed, this, &Styler::on_update_style);
        if ( auto new_col = qobject_cast<glaxnimate::model::NamedColor*>(new_use) )
            reset = new_col->color.get();
    }

    if ( reset.isValid() )
        color.set(reset);

    Q_EMIT use_changed(new_use);
    Q_EMIT use_changed_from(old_use, new_use);
}

void glaxnimate::model::Styler::on_update_style()
{
    Q_EMIT property_changed(&use, use.value());
}

QBrush glaxnimate::model::Styler::brush(FrameTime t) const
{
    if ( use.get() )
        return use->brush_style(t);
    return color.get_at(t);
}
