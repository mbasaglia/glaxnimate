/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "animation_container.hpp"
#include "model/factory.hpp"
#include "model/document.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::AnimationContainer)

bool glaxnimate::model::AnimationContainer::time_visible(glaxnimate::model::FrameTime time) const
{
    return first_frame.get() <= time && time <= last_frame.get();
}

bool glaxnimate::model::AnimationContainer::time_visible() const
{
    return time_visible(time());
}

void glaxnimate::model::AnimationContainer::set_time(glaxnimate::model::FrameTime t)
{
    bool old_visible = time_visible();
    Object::set_time(t);
    bool new_visible = time_visible();
    if ( old_visible != new_visible )
    {
        emit time_visible_changed(new_visible);
        emit document()->graphics_invalidated();
    }
}

void glaxnimate::model::AnimationContainer::on_first_frame_changed(float x)
{
    emit time_visible_changed(time_visible());
    emit first_frame_changed(x);
}

void glaxnimate::model::AnimationContainer::on_last_frame_changed(float x)
{
    emit time_visible_changed(time_visible());
    emit last_frame_changed(x);
}

float glaxnimate::model::AnimationContainer::duration() const
{
    return last_frame.get() - first_frame.get();
}

QString glaxnimate::model::AnimationContainer::type_name_human() const
{
    return tr("Animation Timing");
}

void glaxnimate::model::AnimationContainer::stretch_time(qreal multiplier)
{
    Object::stretch_time(multiplier);
    first_frame.set(first_frame.get() * multiplier);
    last_frame.set(last_frame.get() * multiplier);
}


bool glaxnimate::model::AnimationContainer::validate_first_frame(int f) const
{
    return f >= 0 && (last_frame.get() == -1 || f < last_frame.get());
}

bool glaxnimate::model::AnimationContainer::validate_last_frame(int f) const
{
    return f >= 0 && f > first_frame.get();
}
