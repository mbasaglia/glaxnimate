#include "styler.hpp"
#include "model/document.hpp"

std::vector<model::ReferenceTarget*> model::Styler::valid_uses() const
{
    return document()->defs()->valid_brush_styles();
}

bool model::Styler::is_valid_use(ReferenceTarget* node) const
{
    return document()->defs()->has_brush_style(node);
}

void model::Styler::on_use_changed(model::BrushStyle* new_use, model::BrushStyle* old_use)
{
    if ( old_use )
        disconnect(old_use, &BrushStyle::style_changed, this, &Styler::on_update_style);

    if ( new_use )
        connect(new_use, &BrushStyle::style_changed, this, &Styler::on_update_style);
}

void model::Styler::on_update_style()
{
    emit property_changed(&use, use.value());
}

