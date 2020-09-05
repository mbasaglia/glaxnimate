#pragma once

#pragma once

#include "shape.hpp"
#include "model/animation/animatable_path.hpp"

namespace model {


class Path : public Shape
{
    GLAXNIMATE_OBJECT

public:
    AnimatablePath path{this, "path"};

    GLAXNIMATE_PROPERTY(bool, closed, false, &Path::closed_changed)

public:
    using Shape::Shape;

    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("draw-bezier-curves");
    }

    QString type_name_human() const override
    {
        return tr("Path");
    }

    math::Bezier to_bezier(FrameTime t) const override
    {
        return path.get_at(t);
    }

    QRectF local_bounding_rect(FrameTime t) const override
    {
        return path.get_at(t).bounding_box();
    }

private:
    void closed_changed(bool closed)
    {
        path.set_closed(closed);
    }
};

} // namespace model

