#pragma once

#include "composition.hpp"
#include "model/animation_container.hpp"


namespace glaxnimate::model {

class MainComposition : public Composition
{
    GLAXNIMATE_OBJECT(MainComposition)

    GLAXNIMATE_SUBOBJECT(AnimationContainer, animation)

    //                  type    name    default  notify                       validate
    GLAXNIMATE_PROPERTY(float,  fps,         60, &MainComposition::fps_changed,     &MainComposition::validate_fps)
    GLAXNIMATE_PROPERTY(int,    width,      512, &MainComposition::width_changed,   &MainComposition::validate_nonzero, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(int,    height,     512, &MainComposition::height_changed,  &MainComposition::validate_nonzero, PropertyTraits::Visual)

public:
    using Composition::Composition;

    QIcon tree_icon() const override
    {
        return QIcon::fromTheme("video-x-generic");
    }

    QString type_name_human() const override { return tr("Animation"); }

signals:
    void fps_changed(float fps);
    void width_changed(int);
    void height_changed(int);

private:
    bool validate_nonzero(int size) const
    {
        return size > 0;
    }

    bool validate_out_point(int p) const
    {
        return p > 0;
    }

    bool validate_fps(float v) const
    {
        return v > 0;
    }
};

} // namespace glaxnimate::model
