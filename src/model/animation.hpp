#pragma once

#include "composition.hpp"

namespace model {

class Animation : public ObjectBase<Animation, Composition>
{
    Q_OBJECT

    GLAXNIMATE_PROPERTY(float,  fps,       60, true, nullptr, &Animation::validate_fps)
    GLAXNIMATE_PROPERTY(int,    in_point,   0, true, nullptr, &Animation::validate_in_point)
    GLAXNIMATE_PROPERTY(int,    out_point,180, true, nullptr, &Animation::validate_out_point)
    GLAXNIMATE_PROPERTY(int,    width,    512, true, nullptr, &Animation::validate_size)
    GLAXNIMATE_PROPERTY(int,    height,   512, true, nullptr, &Animation::validate_size)

public:
    using Ctor::Ctor;

    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("video-x-generic");
    }

    graphics::DocumentNodeGraphicsItem* docnode_make_graphics_item() override;

    QString type_name_human() const override { return tr("Animation"); }

private:
    bool validate_size(int size) const
    {
        return size > 0;
    }

    bool validate_in_point(int p) const
    {
        return p > 0 && p < out_point.get();
    }

    bool validate_out_point(int p) const
    {
        return p > in_point.get();
    }

    bool validate_fps(float v) const
    {
        return v > 0;
    }
};

} // namespace model
