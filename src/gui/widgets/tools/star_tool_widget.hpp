/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "model/shapes/polystar.hpp"

#include "shape_tool_widget.hpp"

namespace glaxnimate::gui {

class StarToolWidget : public ShapeToolWidget
{
public:
    StarToolWidget(QWidget* parent = nullptr);

    model::PolyStar::StarType star_type() const;

    double spoke_ratio() const;

    int points();

private:
    class Private;
};

} // namespace glaxnimate::gui
