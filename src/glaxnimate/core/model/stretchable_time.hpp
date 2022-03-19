#pragma once

#include "glaxnimate/core/model/object.hpp"
#include "glaxnimate/core/model/property/property.hpp"

namespace glaxnimate::model {


class StretchableTime : public Object
{
    GLAXNIMATE_OBJECT(StretchableTime)
    GLAXNIMATE_PROPERTY(float, start_time, 0, &StretchableTime::timing_changed, {}, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(float, stretch, 1,    &StretchableTime::timing_changed, {}, PropertyTraits::Visual)

public:
    using Object::Object;

    float time_to_local(float global) const;
    float time_from_local(float local) const;

    QString type_name_human() const override;

private:
    bool validate_stretch(float stretch);

signals:
    void timing_changed();
};


} // model
