#pragma once

#include "shape.hpp"
#include <QRawFont>

namespace model {

class TextShape : public ShapeElement
{
    GLAXNIMATE_OBJECT(TextShape)
    GLAXNIMATE_PROPERTY(QString, text, {})
    GLAXNIMATE_ANIMATABLE(QPointF, position, QPointF())
    GLAXNIMATE_PROPERTY(float, size, 32)

public:
    using ShapeElement::ShapeElement;
    void add_shapes(FrameTime t, math::bezier::MultiBezier& bez) const override;
    QPainterPath to_painter_path(FrameTime t) const override;
    QRawFont font() const;
    QIcon tree_icon() const override;
    QRectF local_bounding_rect(FrameTime t) const override;
};

} // namespace model
