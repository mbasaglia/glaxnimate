#pragma once

#include "model/object.hpp"
#include "model/property/object_list_property.hpp"
#include "named_color.hpp"

namespace model {

class Defs : public ObjectBase<Defs, Object>
{
    GLAXNIMATE_OBJECT

    GLAXNIMATE_PROPERTY_LIST(NamedColor, colors, &Defs::on_color_added, &Defs::on_color_removed, {}, {}, {}, {})

public:
    using Ctor::Ctor;

    std::vector<ReferenceTarget*> valid_brush_styles() const;
    bool is_valid_brush_style(ReferenceTarget* style) const;

    Q_INVOKABLE model::ReferenceTarget* find_by_uuid(const QUuid& n) const;

signals:
    void color_added(int position, model::NamedColor* color);
    void color_removed(int position);
    void color_changed(int position, model::NamedColor* color);

private:
    void on_color_added(NamedColor* color, int position);
    void on_color_removed(NamedColor* color, int position);
};

} // namespace model
