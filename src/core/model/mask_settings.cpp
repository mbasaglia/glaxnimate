#include "mask_settings.hpp"

GLAXNIMATE_OBJECT_IMPL(model::MaskSettings)

QString model::MaskSettings::type_name_human() const
{
    return tr("Mask");
}
