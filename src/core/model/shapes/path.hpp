/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#pragma once

#include "shape.hpp"
#include "model/animation/animatable_path.hpp"

namespace glaxnimate::model {


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
        auto bezier = shape.get_at(t);

        if ( reversed.get() )
            bezier.reverse();

        return bezier;
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

Q_SIGNALS:
    void shape_changed(const math::bezier::Bezier& bez);
};

} // namespace glaxnimate::model

