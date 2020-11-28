#pragma once

#include "model/object.hpp"
#include "model/property/property.hpp"

namespace model {


/**
 * \brief Base class for document nodes that enclose an animation
 */
class AnimationContainer: public Object
{
    GLAXNIMATE_OBJECT(AnimationContainer)
    GLAXNIMATE_PROPERTY(float, first_frame,  0, &AnimationContainer::on_first_frame_changed, &AnimationContainer::validate_first_frame, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(float, last_frame, 180, &AnimationContainer::on_last_frame_changed,  &AnimationContainer::validate_last_frame,  PropertyTraits::Visual)
    Q_PROPERTY(bool time_visible READ time_visible)
    Q_PROPERTY(float duration READ duration)

public:
    using Object::Object;

    /**
     * \brief Whether time() is within first/last frame
     */
    bool time_visible() const;

    bool time_visible(FrameTime t) const;

    void set_time(FrameTime t) override;

    float duration() const;

signals:
    void first_frame_changed(float);
    void last_frame_changed(float);
    void time_visible_changed(bool visible);

private slots:
    void on_first_frame_changed(float);
    void on_last_frame_changed(float);

private:
    bool validate_first_frame(int f) const
    {
        return f >= 0 && f < last_frame.get();
    }

    bool validate_last_frame(int f) const
    {
        return f > first_frame.get();
    }
};


class StretchableAnimation : public AnimationContainer
{
    GLAXNIMATE_OBJECT(StretchableAnimation)
    GLAXNIMATE_PROPERTY(float, start_time, 0, &StretchableAnimation::timing_changed, {}, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(float, stretch, 1,    &StretchableAnimation::timing_changed, {}, PropertyTraits::Visual)

public:
    using AnimationContainer::AnimationContainer;

    float time_to_local(float global) const;
    float time_from_local(float local) const;

private:
    bool validate_stretch(float stretch);

signals:
    void timing_changed();
};

} // namespace model
