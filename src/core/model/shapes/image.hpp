#pragma once

#include "model/defs/bitmap.hpp"
#include "model/shapes/shapes.hpp"
#include "model/property/reference_property.hpp"

namespace model {

class Image : public ShapeElement
{
    GLAXNIMATE_OBJECT(Image)

    GLAXNIMATE_SUBOBJECT(Transform, transform)
    GLAXNIMATE_PROPERTY_REFERENCE(Bitmap, image, &Image::valid_images, &Image::is_valid_image, &Image::on_image_changed)

public:
    Image(model::Document* doc);

    void add_shapes(FrameTime, math::MultiBezier&) const override { return; }

    QIcon docnode_icon() const override { return QIcon::fromTheme("x-shape-image"); }


    QString type_name_human() const override
    {
        return tr("Image");
    }

    QRectF local_bounding_rect(FrameTime t) const override;

    QTransform local_transform_matrix(model::FrameTime t) const override;

protected:
    void on_paint(QPainter* p, FrameTime t, PaintMode) const override;

private slots:
    void on_transform_matrix_changed();

private:
    std::vector<ReferenceTarget*> valid_images() const;
    bool is_valid_image(ReferenceTarget* node) const;
    void on_image_changed(Bitmap* new_use, Bitmap* old_use);
    void on_update_image();
};

} // namespace model
