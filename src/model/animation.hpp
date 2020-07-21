#pragma once

#include "composition.hpp"

namespace model {

class Animation : public DocumentNodeBase<Animation, Composition>
{
    Q_OBJECT

public:
    Property<QString> version{this, "version", "v", ""};
    Property<float> frame_rate{this, "frame_rate", "fr", 60};
    Property<float> in_point{this, "in_point", "ip", 0};
    Property<float> out_point{this, "out_point", "op", 180};
    Property<int> width{this, "width", "w", 512};
    Property<int> height{this, "height", "h", 512};
    // ddd
    // assets
    // comps
    // fonts
    // chars
    // markers
    // motion_blur

    using Ctor::Ctor;

    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("video-x-generic");
    }

    graphics::DocumentNodeGraphicsItem* docnode_make_graphics_item() override;
};

} // namespace model
