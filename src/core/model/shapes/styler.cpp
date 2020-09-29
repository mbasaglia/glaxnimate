#include "styler.hpp"
#include "model/document.hpp"
#include "model/defs/named_color.hpp"

std::vector<model::ReferenceTarget*> model::Styler::valid_uses() const
{
    return document()->defs()->valid_brush_styles();
}

bool model::Styler::is_valid_use(ReferenceTarget* node) const
{
    return document()->defs()->is_valid_brush_style(node);
}

void model::Styler::on_use_changed(model::BrushStyle* new_use, model::BrushStyle* old_use)
{
    QColor reset;

    if ( old_use )
    {
        old_use->remove_user(&use);
        disconnect(old_use, &BrushStyle::style_changed, this, &Styler::on_update_style);
        if ( auto old_col = qobject_cast<model::NamedColor*>(old_use) )
            reset = old_col->color.get();
    }

    if ( new_use )
    {
        connect(new_use, &BrushStyle::style_changed, this, &Styler::on_update_style);
        new_use->add_user(&use);
        if ( auto new_col = qobject_cast<model::NamedColor*>(new_use) )
            reset = new_col->color.get();
    }

    if ( reset.isValid() )
        color.set(reset);
}

void model::Styler::on_update_style()
{
    emit property_changed(&use, use.value());
}

QBrush model::Styler::brush(FrameTime t) const
{
    if ( use.get() )
        return use->brush_style(t);
    return color.get_at(t);
}
