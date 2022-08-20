#pragma once

#include "path_modifier.hpp"
#include "stroke.hpp"

namespace glaxnimate::model {

class OffsetPath : public StaticOverrides<OffsetPath, PathModifier>
{
    GLAXNIMATE_OBJECT(OffsetPath)
    GLAXNIMATE_ANIMATABLE(float, amount, 0)
    GLAXNIMATE_ANIMATABLE(float, miter_limit, 100, {}, 0)
    GLAXNIMATE_PROPERTY(Stroke::Join, join, Stroke::RoundJoin, nullptr, nullptr, PropertyTraits::Visual)

public:
    using Ctor::Ctor;

    static QIcon static_tree_icon();
    static QString static_type_name_human();

    math::bezier::MultiBezier process(FrameTime t, const math::bezier::MultiBezier& mbez) const override;

protected:
    bool process_collected() const override;

};

} // namespace glaxnimate::model
