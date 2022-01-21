#pragma once

#include "model/object.hpp"
#include "model/property/reference_property.hpp"
#include "model/animation/frame_time.hpp"
#include "model/shapes/shape.hpp"

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
