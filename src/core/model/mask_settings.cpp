#include "mask_settings.hpp"

#include "model/document.hpp"
#include "model/defs/defs.hpp"
#include "model/defs/precomposition.hpp"

GLAXNIMATE_OBJECT_IMPL(model::MaskSettings)

std::vector<model::ReferenceTarget*> model::MaskSettings::valid_masks() const
{
    return document()->defs()->masks->shapes.valid_reference_values(true);
}

bool model::MaskSettings::is_valid_mask(model::ReferenceTarget* node) const
{
    return document()->defs()->masks->shapes.is_valid_reference_value(node, true);
}

QPainterPath model::MaskSettings::to_clip(FrameTime time, const QTransform& target_transform) const
{
    if ( !mask.get() )
        return {};

    QPainterPath clip = mask->to_clip(time);

    if ( lock_transform.get() )
        return target_transform.map(clip);

    return clip;
}

bool model::MaskSettings::has_mask() const
{
    return mask.get();
}

QString model::MaskSettings::type_name_human() const
{
    return tr("Mask");
}
