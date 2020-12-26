#pragma once

#include "model/object.hpp"
#include "model/property/object_list_property.hpp"
#include "model/property/sub_object_property.hpp"
#include "named_color.hpp"
#include "bitmap.hpp"
#include "gradient.hpp"
#include "precomposition.hpp"


namespace model {

class Defs : public Object
{
    GLAXNIMATE_OBJECT(Defs)

    GLAXNIMATE_PROPERTY_LIST(model::NamedColor, colors, &Defs::on_color_added, &Defs::on_color_removed, {}, {}, {}, {})
    GLAXNIMATE_PROPERTY_LIST(model::Bitmap, images, &Defs::on_added, &Defs::on_removed, {}, {}, {}, {})
    GLAXNIMATE_PROPERTY_LIST(model::GradientColors, gradient_colors,
        &Defs::on_gradient_colors_added,
        &Defs::on_gradient_colors_removed,
        &Defs::gradient_add_begin,
        &Defs::gradient_remove_begin,
        &Defs::gradient_move_begin,
        &Defs::gradient_move_end
    )
    GLAXNIMATE_PROPERTY_LIST(model::Gradient, gradients, &Defs::on_added, &Defs::on_removed, {}, {}, {}, {})
    GLAXNIMATE_PROPERTY_LIST(model::Precomposition, precompositions,
        &Defs::on_precomp_added,
        &Defs::on_precomp_removed,
        &Defs::precomp_add_begin,
        &Defs::precomp_remove_begin,
        &Defs::precomp_move_begin,
        &Defs::precomp_move_end
    )

public:
    explicit Defs(Document* document);

    Q_INVOKABLE model::ReferenceTarget* find_by_uuid(const QUuid& n) const;
    Q_INVOKABLE model::NamedColor* add_color(const QColor& color, const QString& name = {});
    Q_INVOKABLE model::Bitmap* add_image_file(const QString& filename, bool embed);
    Q_INVOKABLE model::Bitmap* add_image(const QImage& image, const QString& store_as = "png");
    Q_INVOKABLE model::GradientColors* add_gradient_colors(int index = -1);
    Q_INVOKABLE model::Gradient* add_gradient(int index = -1);

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

    void precomp_add_begin(int row);
    void precomp_add_end(Precomposition* node, int row);
    void precomp_remove_begin(int row);
    void precomp_remove_end(Precomposition* node);
    void precomp_move_begin(int from, int to);
    void precomp_move_end(Precomposition* node, int from, int to);

private:
    void on_color_added(NamedColor* color, int position);
    void on_color_removed(NamedColor* color, int position);

    void on_gradient_colors_added(GradientColors* color);
    void on_gradient_colors_removed(GradientColors* color);

    void on_added(AssetBase* def);
    void on_removed(AssetBase* def);

    void on_precomp_added(Precomposition* obj, int row);
    void on_precomp_removed(Precomposition* obj);
};

} // namespace model
