#include "defs.hpp"
#include "model/document.hpp"

GLAXNIMATE_OBJECT_IMPL(model::Defs)

std::vector<model::ReferenceTarget *> model::Defs::valid_brush_styles() const
{
    std::vector<model::ReferenceTarget *> res;
    res.reserve(colors.size());
    for ( const auto& c : colors )
        res.push_back(c.get());
    return res;
}

bool model::Defs::has_brush_style(model::ReferenceTarget* style) const
{
    for ( const auto& c : colors )
        if ( c.get() == style )
            return true;
    return false;
}
