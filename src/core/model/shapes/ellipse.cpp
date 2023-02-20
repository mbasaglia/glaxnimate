/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ellipse.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Ellipse)


QIcon glaxnimate::model::Ellipse::tree_icon() const
{
    return QIcon::fromTheme("draw-ellipse");
}

QString glaxnimate::model::Ellipse::type_name_human() const
{
    return tr("Ellipse");
}

glaxnimate::math::bezier::Bezier glaxnimate::model::Ellipse::to_bezier(FrameTime t) const
{
    QSizeF sz = size.get_at(t);

    auto bezier = math::EllipseSolver(position.get_at(t), QPointF(sz.width()/2, sz.height()/2), 0).to_bezier(-math::pi/2, math::tau);

    if ( reversed.get() )
        bezier.reverse();

    return bezier;
}

QRectF glaxnimate::model::Ellipse::local_bounding_rect(FrameTime t) const
{
    QSizeF sz = size.get_at(t);
    return QRectF(position.get_at(t) - QPointF(sz.width()/2, sz.height()/2), sz);
}
