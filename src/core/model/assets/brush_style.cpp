/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "brush_style.hpp"

QIcon glaxnimate::model::BrushStyle::instance_icon() const
{
    if ( icon.isNull() )
    {
        icon = QPixmap(32, 32);
        fill_icon(icon);
    }
    return icon;
}

QBrush glaxnimate::model::BrushStyle::constrained_brush_style(FrameTime t, const QRectF& ) const
{
    return brush_style(t);
}
