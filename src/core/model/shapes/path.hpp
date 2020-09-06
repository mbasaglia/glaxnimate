#pragma once

#pragma once

#include "shape.hpp"
#include "model/animation/animatable_path.hpp"

namespace model {


class Path : public Shape
{
    GLAXNIMATE_OBJECT
    using BezierPointType = math::BezierPointType;
    Q_ENUM(BezierPointType)

public:
    AnimatablePath shape{this, "shape", &Path::shape_changed};

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
        return shape.get_at(t);
    }

    QRectF local_bounding_rect(FrameTime t) const override
    {
        return shape.get_at(t).bounding_box();
    }

private:
    void closed_changed(bool closed)
    {
        shape.set_closed(closed);
    }

signals:
    void shape_changed(const math::Bezier& bez);
};

} // namespace model

