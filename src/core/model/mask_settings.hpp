#pragma once

#include "model/object.hpp"
#include "model/property/reference_property.hpp"
#include "model/animation/frame_time.hpp"
#include "model/shapes/shape.hpp"

namespace model {

class MaskSettings : public Object
{
    GLAXNIMATE_OBJECT(MaskSettings)

    GLAXNIMATE_PROPERTY(bool, mask, false, {}, {}, PropertyTraits::Visual)
//     GLAXNIMATE_PROPERTY(bool, invert, false, {}, {}, PropertyTraits::Visual)

public:
    using Object::Object;

    QString type_name_human() const override;

    bool has_mask() const { return mask.get(); }
};

} // namespace model
