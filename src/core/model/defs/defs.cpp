#include "defs.hpp"
#include "model/document.hpp"

GLAXNIMATE_OBJECT_IMPL(model::Defs)

std::vector<model::ReferenceTarget *> model::Defs::valid_brush_styles() const
{
    std::vector<model::ReferenceTarget *> res;
    res.reserve(colors.size()+1);
    res.push_back(nullptr);
    for ( const auto& c : colors )
        res.push_back(c.get());
    return res;
}

bool model::Defs::is_valid_brush_style(model::ReferenceTarget* style) const
{
    if ( style == nullptr )
        return true;

    for ( const auto& c : colors )
        if ( c.get() == style )
            return true;
    return false;
}

void model::Defs::on_color_added(model::NamedColor* color, int position)
{
    connect(color, &Object::property_changed, this, [position, color, this]{
        emit color_changed(position, color);
    });
    emit color_added(position, color);
}

void model::Defs::on_color_removed(model::NamedColor* color, int position)
{
    disconnect(color, nullptr, this, nullptr);
    emit color_removed(position);
}

