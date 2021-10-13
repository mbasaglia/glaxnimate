#pragma once

#include "path_modifier.hpp"

namespace glaxnimate::model {

class Trim : public StaticOverrides<Trim, PathModifier>
{
    GLAXNIMATE_OBJECT(Trim)
    GLAXNIMATE_ANIMATABLE(float, start, 0, {}, 0, 1, false, PropertyTraits::Percent)
    GLAXNIMATE_ANIMATABLE(float, end, 1, {}, 0, 1, false, PropertyTraits::Percent)
    GLAXNIMATE_ANIMATABLE(float, offset, 0, {}, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max(), false, PropertyTraits::Percent)

public:
    enum MultipleShapes
    {
        Simultaneously = 1,
        Individually = 2,
    };

    Q_ENUM(MultipleShapes)

    GLAXNIMATE_PROPERTY(MultipleShapes, multiple, Individually, {}, {}, PropertyTraits::Visual)


public:
    using Ctor::Ctor;

    static QIcon static_tree_icon();
    static QString static_type_name_human();

    math::bezier::MultiBezier process(FrameTime t, const math::bezier::MultiBezier& mbez) const override;

protected:
    bool process_collected() const override;

};

} // namespace glaxnimate::model

