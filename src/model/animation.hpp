#pragma once

#include "composition.hpp"

namespace model {

class Animation : public Composition
{
public:
    Property<QString> version{this, "version", "v", ""};
    Property<float> frame_rate{this, "frame_rate", "fr", 60};
    Property<float> in_point{this, "in_point", "ip", 0};
    Property<float> out_point{this, "out_point", "op", 180};
    Property<int> width{this, "width", "w", 512};
    Property<int> height{this, "height", "h", 512};
    Property<QString> name{this, "name", "nm", ""};
    // ddd
    // assets
    // comps
    // fonts
    // chars
    // markers
    // motion_blur
};

} // namespace model
