#pragma once

#include "layer.hpp"

namespace model {

class SolidColorLayer : public detail::BaseLayerProps<SolidColorLayer>
{
    GLAXNIMATE_OBJECT

    //                  type    name    default     notify                                   validate
    GLAXNIMATE_PROPERTY(float,  width,  0,          &SolidColorLayer::bounding_rect_changed, &SolidColorLayer::positive, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(float,  height, 0,          &SolidColorLayer::bounding_rect_changed, &SolidColorLayer::positive, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(QColor, color,  Qt::white, {}, {}, PropertyTraits::Visual)
public:
    SolidColorLayer(Document* doc, Composition* composition);

    QRectF local_bounding_rect(FrameTime t) const override;

    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("object-fill");
    }

    QString type_name_human() const override { return tr("Solid Color Layer"); }

    bool docnode_selection_container() const override { return false; }

protected:
    void on_paint_untransformed(QPainter*, FrameTime) const override;

private:
    bool positive(float x) const
    {
        return x > 0;
    }
};

} // namespace model
