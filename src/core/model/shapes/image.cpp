#include "image.hpp"
#include "model/document.hpp"
#include "model/defs/defs.hpp"

GLAXNIMATE_OBJECT_IMPL(model::Image)

bool model::Image::is_valid_image(model::ReferenceTarget* node) const
{
    return document()->defs()->is_valid_image(node);
}

std::vector<model::ReferenceTarget *> model::Image::valid_images() const
{
    return document()->defs()->valid_images();
}

QRectF model::Image::local_bounding_rect(model::FrameTime) const
{
    if ( !image.get() )
        return {};
    return QRectF(0, 0, image->width.get(), image->height.get());
}

void model::Image::on_paint(QPainter* p, model::FrameTime, model::DocumentNode::PaintMode) const
{
    if ( image.get() )
        image->paint(p);
}

void model::Image::on_image_changed(model::Bitmap* new_use, model::Bitmap* old_use)
{
    if ( old_use )
        disconnect(old_use, &Bitmap::loaded, this, &Image::on_update_image);

    if ( new_use )
        connect(new_use, &Bitmap::loaded, this, &Image::on_update_image);
}

void model::Image::on_update_image()
{
    emit property_changed(&image, {});
}