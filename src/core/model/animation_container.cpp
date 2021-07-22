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
