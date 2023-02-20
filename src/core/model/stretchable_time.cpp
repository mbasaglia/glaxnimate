/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "stretchable_time.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::StretchableTime)

bool glaxnimate::model::StretchableTime::validate_stretch(float stretch)
{
    return stretch > 0;
}

float glaxnimate::model::StretchableTime::time_to_local(float global) const
{
    return (global - start_time.get()) / stretch.get();
}

float glaxnimate::model::StretchableTime::time_from_local(float local) const
{
    return local * stretch.get() + start_time.get();
}

QString glaxnimate::model::StretchableTime::type_name_human() const
{
    return tr("Timing");
}
