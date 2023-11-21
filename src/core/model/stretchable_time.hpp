/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "model/object.hpp"
#include "model/property/property.hpp"

namespace glaxnimate::model {


class StretchableTime : public Object
{
    GLAXNIMATE_OBJECT(StretchableTime)
    GLAXNIMATE_PROPERTY(float, start_time, 0, &StretchableTime::timing_changed, {}, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(float, stretch, 1,    &StretchableTime::timing_changed, {}, PropertyTraits::Visual|PropertyTraits::Percent)

public:
    using Object::Object;

    float time_to_local(float global) const;
    float time_from_local(float local) const;

    QString type_name_human() const override;

private:
    bool validate_stretch(float stretch);

Q_SIGNALS:
    void timing_changed();
};


} // model
