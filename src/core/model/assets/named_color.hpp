/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "brush_style.hpp"
#include "model/animation/animatable.hpp"

namespace glaxnimate::model {

class NamedColor : public BrushStyle
{
    GLAXNIMATE_OBJECT(NamedColor)

    GLAXNIMATE_ANIMATABLE(QColor, color, QColor(0, 0, 0), &NamedColor::invalidate_icon)

public:
    using BrushStyle::BrushStyle;

    QString type_name_human() const override;

    QBrush brush_style(FrameTime t) const override;

    bool remove_if_unused(bool clean_lists) override;

protected:
    void fill_icon(QPixmap& icon) const override;

};

} // namespace glaxnimate::model
