#pragma once

#include "glaxnimate/core/model/object.hpp"
#include "glaxnimate/core/model/property/reference_property.hpp"
#include "glaxnimate/core/model/animation/frame_time.hpp"
#include "glaxnimate/core/model/shapes/shape.hpp"

namespace glaxnimate::model {

class MaskSettings : public Object
{
    GLAXNIMATE_OBJECT(MaskSettings)

public:
    enum MaskMode
    {
        NoMask = 0,
        Alpha = 1,
    };
    Q_ENUM(MaskMode)

    GLAXNIMATE_PROPERTY(MaskMode, mask, NoMask, {}, {}, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(bool, inverted, false, {}, {}, PropertyTraits::Visual)

public:
    using Object::Object;

    QString type_name_human() const override;

    bool has_mask() const { return mask.get(); }
};

} // namespace glaxnimate::model
