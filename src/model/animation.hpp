#pragma once

#include "composition.hpp"

namespace model {

class Animation : public ObjectBase<Animation, Composition>
{
    GLAXNIMATE_OBJECT

    //                  type    name    default  notify                       validate
    GLAXNIMATE_PROPERTY(float,  fps,         60, &Animation::fps_changed,     &Animation::validate_fps)
    GLAXNIMATE_PROPERTY(int,    width,      512, &Animation::width_changed,   &Animation::validate_nonzero, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(int,    height,     512, &Animation::height_changed,  &Animation::validate_nonzero, PropertyTraits::Visual)

public:
    using Ctor::Ctor;

    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("video-x-generic");
    }

    graphics::DocumentNodeGraphicsItem* docnode_make_graphics_item() override;
    std::vector<std::unique_ptr<QGraphicsItem>> docnode_make_graphics_editor() override;

    QString type_name_human() const override { return tr("Animation"); }

    QRectF local_bounding_rect(FrameTime) const override
    {
        return QRectF(0, 0, width.get(), height.get());
    }

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

} // namespace model
