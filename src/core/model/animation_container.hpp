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
    GLAXNIMATE_PROPERTY(int,    first_frame,  0, &AnimationContainer::first_frame_changed, &AnimationContainer::validate_first_frame, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(int,    last_frame, 180, &AnimationContainer::last_frame_changed,  &AnimationContainer::validate_last_frame,  PropertyTraits::Visual)

public:
    using Ctor::Ctor;

signals:
    void first_frame_changed(int);
    void last_frame_changed(int);

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
