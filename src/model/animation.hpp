#pragma once

#include "composition.hpp"

namespace model {

class Animation : public DocumentNodeBase<Animation, Composition>
{
    Q_OBJECT

public:
    Property<float> frame_rate{this, "frame_rate", 60};
    Property<float> in_point{this, "in_point", 0};
    Property<float> out_point{this, "out_point", 180};
    Property<int> width{this, "width", 512};
    Property<int> height{this, "height", 512};
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

    QString type_name_human() const override { return tr("Animation"); }
};

} // namespace model
