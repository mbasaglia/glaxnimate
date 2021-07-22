#pragma once

#include "model/assets/bitmap.hpp"
#include "model/shapes/shape.hpp"
#include "model/transform.hpp"
#include "model/property/reference_property.hpp"
#include "model/property/sub_object_property.hpp"

namespace glaxnimate::model {

class Image : public ShapeElement
{
    GLAXNIMATE_OBJECT(Image)

    GLAXNIMATE_SUBOBJECT(model::Transform, transform)
    GLAXNIMATE_PROPERTY_REFERENCE(model::Bitmap, image, &Image::valid_images, &Image::is_valid_image, &Image::on_image_changed)

public:
    Image(model::Document* doc);

    void add_shapes(FrameTime, math::bezier::MultiBezier&, const QTransform&) const override;
    QPainterPath to_painter_path(FrameTime t) const override;

    QIcon tree_icon() const override;
    QString type_name_human() const override;
    QRectF local_bounding_rect(FrameTime t) const override;
    QTransform local_transform_matrix(model::FrameTime t) const override;

protected:
    void on_paint(QPainter* p, FrameTime t, PaintMode, model::Modifier*) const override;

private slots:
    void on_transform_matrix_changed();

private:
    std::vector<DocumentNode*> valid_images() const;
    bool is_valid_image(DocumentNode* node) const;
    void on_image_changed(Bitmap* new_use, Bitmap* old_use);
    void on_update_image();
};

} // namespace glaxnimate::model
