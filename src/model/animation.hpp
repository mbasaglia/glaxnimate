#pragma once

#include "composition.hpp"

namespace model {

class Animation : public ObjectBase<Animation, Composition>
{
    Q_OBJECT

    GLAXNIMATE_PROPERTY(float,  fps,         60, true, &Animation::fps_changed, &Animation::validate_fps)
    GLAXNIMATE_PROPERTY(int,    width,      512, true, nullptr,                 &Animation::validate_nonzero)
    GLAXNIMATE_PROPERTY(int,    height,     512, true, nullptr,                 &Animation::validate_nonzero)

public:
    using Ctor::Ctor;

    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("video-x-generic");
    }

    graphics::DocumentNodeGraphicsItem* docnode_make_graphics_item() override;

    QString type_name_human() const override { return tr("Animation"); }

    QRectF bounding_rect(FrameTime) const override
    {
        return QRectF(0, 0, width.get(), height.get());
    }

signals:
    void fps_changed(float fps);

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

} // namespace model
