#pragma once

#include "model/object.hpp"
#include "model/property/reference_property.hpp"
#include "model/animation/frame_time.hpp"
#include "model/shapes/shape.hpp"

namespace model {

class MaskSettings : public Object
{
    GLAXNIMATE_OBJECT(MaskSettings)

    GLAXNIMATE_PROPERTY_REFERENCE(model::ShapeElement, mask, &MaskSettings::valid_masks, &MaskSettings::is_valid_mask, &MaskSettings::mask_changed)
    GLAXNIMATE_PROPERTY(bool, lock_transform, true, &MaskSettings::lock_transform_changed, {}, PropertyTraits::Visual)
//     GLAXNIMATE_PROPERTY(bool, invert, false, {}, {}, PropertyTraits::Visual)

public:
    using Object::Object;

    QString type_name_human() const override;

    QPainterPath to_clip(FrameTime time, const QTransform& target_transform) const;

    bool has_mask() const;

private:
    std::vector<ReferenceTarget*> valid_masks() const;
    bool is_valid_mask(ReferenceTarget* node) const;

signals:
    void lock_transform_changed(bool);
    void mask_changed(model::ShapeElement* new_mask, model::ShapeElement* old_mask);
};

} // namespace model
