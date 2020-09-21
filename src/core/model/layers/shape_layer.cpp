#include "shape_layer.hpp"
#include "model/document.hpp"

GLAXNIMATE_OBJECT_IMPL(model::ShapeLayer)

QRectF model::ShapeLayer::local_bounding_rect(FrameTime t) const
{
    if ( shapes.empty() )
        return QRectF(QPointF(0, 0), QSizeF(document()->size()));
    return shapes.bounding_rect(t);
}
