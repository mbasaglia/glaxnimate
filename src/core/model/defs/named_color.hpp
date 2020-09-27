#pragma once

#include "brush_style.hpp"
#include "model/animation/animatable.hpp"

namespace model {

class NamedColor : public ObjectBase<NamedColor, BrushStyle>
{
    GLAXNIMATE_OBJECT

    GLAXNIMATE_PROPERTY(QString, name, "")
    GLAXNIMATE_ANIMATABLE(QColor, color, QColor(0, 0, 0), &NamedColor::invalidate_icon)

public:
    using Ctor::Ctor;

    QString object_name() const override;

    QString type_name_human() const override;

    QBrush brush_style(FrameTime t) const override;

protected:
    void fill_icon(QPixmap& icon) const override;

};

} // namespace model
