/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mask_settings.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::MaskSettings)

QString glaxnimate::model::MaskSettings::type_name_human() const
{
    return tr("Mask");
}
