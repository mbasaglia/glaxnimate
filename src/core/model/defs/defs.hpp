#pragma once

#include "model/object.hpp"
#include "model/property/object_list_property.hpp"
#include "named_color.hpp"

namespace model {

class Defs : public ObjectBase<Defs, Object>
{
    GLAXNIMATE_OBJECT

    GLAXNIMATE_PROPERTY_LIST(NamedColor, colors, &Defs::color_added, {}, {}, &Defs::color_removed, {}, {})

public:
    using Ctor::Ctor;

    std::vector<ReferenceTarget*> valid_brush_styles() const;
    bool has_brush_style(ReferenceTarget* style) const;

signals:
    void color_added(NamedColor* color, int position);
    void color_removed(int pos);
};

} // namespace model
