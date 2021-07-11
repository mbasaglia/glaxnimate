#pragma once
#ifdef _HAX_FUCKING_QMAKE_I_HATE_YOU_
    class IAlsoHateLupdate{Q_OBJECT};
#endif

#pragma once

#include "shape.hpp"
#include "model/animation/animatable_path.hpp"

namespace model {


class Path : public Shape
{
    GLAXNIMATE_OBJECT(Path)
    using PointType = math::bezier::PointType;
    Q_ENUM(PointType)

public:
    GLAXNIMATE_ANIMATABLE(math::bezier::Bezier, shape, &Path::shape_changed)

    GLAXNIMATE_PROPERTY(bool, closed, false, &Path::closed_changed)

public:
    using Shape::Shape;

    QIcon tree_icon() const override
    {
        return QIcon::fromTheme("draw-bezier-curves");
    }

    QString type_name_human() const override
    {
        return tr("Path");
    }

    math::bezier::Bezier to_bezier(FrameTime t) const override
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
    void shape_changed(const math::bezier::Bezier& bez);
};

} // namespace model

