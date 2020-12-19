#pragma once

#include "model/object.hpp"
#include "model/property/reference_property.hpp"
#include "model/animation/frame_time.hpp"

namespace model {

class MaskSettings : public Object
{
    GLAXNIMATE_OBJECT(MaskSettings)

    GLAXNIMATE_PROPERTY_REFERENCE(model::ShapeElement, mask, &MaskSettings::valid_masks, &MaskSettings::is_valid_mask)
    GLAXNIMATE_PROPERTY(bool, lock_transform, true, {}, {}, PropertyTraits::Visual)
//     GLAXNIMATE_PROPERTY(bool, invert, false, {}, {}, PropertyTraits::Visual)

public:
    using Object::Object;

    QString type_name_human() const override;

    QPainterPath to_clip(FrameTime time, const QTransform& target_transform) const;

    bool has_mask() const;

private:
    std::vector<ReferenceTarget*> valid_masks() const;
    bool is_valid_mask(ReferenceTarget* node) const;
};

} // namespace model
