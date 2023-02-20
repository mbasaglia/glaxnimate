/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "shape.hpp"

namespace glaxnimate::model {


class PolyStar : public Shape
{
    GLAXNIMATE_OBJECT(PolyStar)

public:
    enum StarType
    {
        Star = 1,
        Polygon = 2,
    };

    Q_ENUM(StarType)

    GLAXNIMATE_PROPERTY(StarType, type, Star, {}, {}, PropertyTraits::Visual)
    GLAXNIMATE_ANIMATABLE(QPointF, position, QPointF())
    GLAXNIMATE_ANIMATABLE(float, outer_radius, 0, {}, 0)
    GLAXNIMATE_ANIMATABLE(float, inner_radius, 0, {}, 0)
    GLAXNIMATE_ANIMATABLE(float, angle, 0, {}, 0, 360, true)
    GLAXNIMATE_ANIMATABLE(int, points, 5)
    GLAXNIMATE_ANIMATABLE(float, outer_roundness, 0, {}, 0, 100, false, PropertyTraits::Percent)
    GLAXNIMATE_ANIMATABLE(float, inner_roundness, 0, {}, 0, 100, false, PropertyTraits::Percent)

public:
    using Shape::Shape;

    QIcon tree_icon() const override
    {
        if ( type.get() == Star )
            return QIcon::fromTheme("draw-star");
        return QIcon::fromTheme("draw-polygon");
    }

    QString type_name_human() const override
    {
        return tr("PolyStar");
    }

    math::bezier::Bezier to_bezier(FrameTime t) const override;

    QRectF local_bounding_rect(FrameTime t) const override
    {
        float radius = qMax(this->outer_radius.get_at(t), this->inner_radius.get_at(t));
        return QRectF(position.get_at(t) - QPointF(radius, radius), QSizeF(radius*2, radius*2));
    }

    bool is_rounded() const;

    static math::bezier::Bezier draw(
        StarType type, const QPointF& pos, float r_in, float r_out,
        float angle_radians, int p, float round_in, float round_out, bool reverse);
};

} // namespace glaxnimate::model

