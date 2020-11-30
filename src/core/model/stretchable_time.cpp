#include "stretchable_time.hpp"

GLAXNIMATE_OBJECT_IMPL(model::StretchableTime)

bool model::StretchableTime::validate_stretch(float stretch)
{
    return stretch > 0;
}

float model::StretchableTime::time_to_local(float global) const
{
    return (global - start_time.get()) / stretch.get();
}

float model::StretchableTime::time_from_local(float local) const
{
    return local * stretch.get() + start_time.get();
}

QString model::StretchableTime::type_name_human() const
{
    return tr("Timing");
}
