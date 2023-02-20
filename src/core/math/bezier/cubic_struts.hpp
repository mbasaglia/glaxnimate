/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "segment.hpp"

namespace glaxnimate::math::bezier {


struct BezierStruts
{
    QPointF B;  ///< Point on a bezier segment
    qreal t;    ///< Bezier parameter in (0,1) (should never be 0 or 1)
    QPointF e1; ///< Linear handles in de casteljau lerp(e1, e2, t) = B
    QPointF e2;
};


QPointF get_quadratic_handle(const math::bezier::BezierSegment& segment, const QPointF& B, qreal t);

math::bezier::BezierSegment cubic_segment_from_struts(
    const math::bezier::BezierSegment& segment,
    const BezierStruts& struts
);

BezierStruts cubic_struts_idealized(const math::bezier::BezierSegment& segment, const QPointF& B);

struct ProjectResult;

BezierStruts cubic_struts_projection(
    const math::bezier::BezierSegment& segment,
    const QPointF& B,
    const math::bezier::ProjectResult& projection
);

} // namespace glaxnimate::math::bezier
