#pragma once

#include "model/object.hpp"
#include "model/property/property.hpp"

namespace model {


/**
 * \brief Base class for document nodes that enclose an animation
 */
class AnimationContainer: public ObjectBase<AnimationContainer, Object>
{
    GLAXNIMATE_OBJECT
    GLAXNIMATE_PROPERTY(int,    first_frame,  0, &AnimationContainer::on_first_frame_changed, &AnimationContainer::validate_first_frame, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(int,    last_frame, 180, &AnimationContainer::on_last_frame_changed,  &AnimationContainer::validate_last_frame,  PropertyTraits::Visual)
    Q_PROPERTY(bool time_visible READ time_visible)

public:
    using Ctor::Ctor;

    /**
     * \brief Whether time() is within first/last frame
     */
    bool time_visible() const;

    bool time_visible(FrameTime t) const;

    void set_time(FrameTime t) override;

signals:
    void first_frame_changed(int);
    void last_frame_changed(int);
    void time_visible_changed(bool visible);

private slots:
    void on_first_frame_changed(int);
    void on_last_frame_changed(int);

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


} // namespace model
