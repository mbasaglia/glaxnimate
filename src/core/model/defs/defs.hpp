#pragma once

#include "model/object.hpp"
#include "model/property/object_list_property.hpp"
#include "named_color.hpp"
#include "bitmap.hpp"
#include "gradient.hpp"


namespace model {

class Defs : public ObjectBase<Defs, Object>
{
    GLAXNIMATE_OBJECT

    GLAXNIMATE_PROPERTY_LIST(NamedColor, colors, &Defs::on_color_added, &Defs::on_color_removed, {}, {}, {}, {})
    GLAXNIMATE_PROPERTY_LIST(Bitmap, images, &Defs::on_added, &Defs::on_removed, {}, {}, {}, {})
    GLAXNIMATE_PROPERTY_LIST(GradientColors, gradient_colors,
        &Defs::on_gradient_colors_added,
        &Defs::on_gradient_colors_removed,
        &Defs::gradient_add_begin,
        &Defs::gradient_remove_begin,
        &Defs::gradient_move_begin,
        &Defs::gradient_move_end
    )
    GLAXNIMATE_PROPERTY_LIST(Gradient, gradients, &Defs::on_added, &Defs::on_removed, {}, {}, {}, {})

public:
    using Ctor::Ctor;

    Q_INVOKABLE model::ReferenceTarget* find_by_uuid(const QUuid& n) const;
    Q_INVOKABLE model::NamedColor* add_color(const QColor& color, const QString& name = {});
    Q_INVOKABLE model::Bitmap* add_image(const QString& filename, bool embed);

signals:
    void color_added(int position, model::NamedColor* color);
    void color_removed(int position);
    void color_changed(int position, model::NamedColor* color);


    void gradient_add_begin(int row);
    void gradient_add_end(GradientColors* node);
    void gradient_remove_begin(int row);
    void gradient_remove_end(GradientColors* node);
    void gradient_move_begin(int from, int to);
    void gradient_move_end(GradientColors* node, int from, int to);

private:
    void on_color_added(NamedColor* color, int position);
    void on_color_removed(NamedColor* color, int position);

    void on_gradient_colors_added(GradientColors* color);
    void on_gradient_colors_removed(GradientColors* color);

    void on_added(Asset* def);
    void on_removed(Asset* def);
};

} // namespace model
