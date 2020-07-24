#pragma once

#include "composition.hpp"

namespace model {

class Animation : public ObjectBase<Animation, Composition>
{
    Q_OBJECT

    GLAXNIMATE_PROPERTY(float, fps, 60)
    GLAXNIMATE_PROPERTY(float, in_point, 0)
    GLAXNIMATE_PROPERTY(float, out_point, 180)
    GLAXNIMATE_PROPERTY(int, width, 512)
    GLAXNIMATE_PROPERTY(int, height, 512)

public:
    using Ctor::Ctor;

    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("video-x-generic");
    }

    graphics::DocumentNodeGraphicsItem* docnode_make_graphics_item() override;

    QString type_name_human() const override { return tr("Animation"); }
};

} // namespace model
