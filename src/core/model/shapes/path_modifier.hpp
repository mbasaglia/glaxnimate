/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "shape.hpp"

namespace glaxnimate::model {

/**
 * \brief Base class for modifiers than only alter the bezier points
 */
class PathModifier : public Modifier
{
    Q_OBJECT

public:
    using Modifier::Modifier;

    std::unique_ptr<ShapeElement> to_path() const override;

protected:
    void on_paint(QPainter* painter, FrameTime t, PaintMode mode, model::Modifier* modifier) const override;

};

} // namespace glaxnimate::model
