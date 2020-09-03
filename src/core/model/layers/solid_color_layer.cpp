#include "solid_color_layer.hpp"

#include <QPainter>

#include "model/composition.hpp"
#include "model/document.hpp"


GLAXNIMATE_OBJECT_IMPL(model::SolidColorLayer)

model::SolidColorLayer::SolidColorLayer ( model::Document* doc, model::Composition* composition )
    : Ctor(doc, composition)
{
    width.set(doc->main_composition()->width.get());
    height.set(doc->main_composition()->height.get());
}


void model::SolidColorLayer::on_paint_untransformed(QPainter* painter, FrameTime time) const
{
    painter->fillRect(local_bounding_rect(time), color.get());
}

QRectF model::SolidColorLayer::local_bounding_rect(FrameTime) const
{
    return QRectF(0, 0, width.get(), height.get());
}

