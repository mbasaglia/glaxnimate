/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "shape.hpp"

#include "model/assets/assets.hpp"
#include "model/property/reference_property.hpp"

namespace glaxnimate::model {

/**
 * \brief Base class for elements that add a style
 */
class Styler : public ShapeOperator
{
    Q_OBJECT

    GLAXNIMATE_ANIMATABLE(QColor, color, QColor())
    GLAXNIMATE_ANIMATABLE(float, opacity, 1, {}, 0, 1, false, PropertyTraits::Percent)
    GLAXNIMATE_PROPERTY_REFERENCE(BrushStyle, use, &Styler::valid_uses, &Styler::is_valid_use, &Styler::on_use_changed)

public:
    using ShapeOperator::ShapeOperator;

    void add_shapes(FrameTime, math::bezier::MultiBezier&, const QTransform&) const override {}

protected:
    QBrush brush(FrameTime t) const;

private:
    std::vector<DocumentNode*> valid_uses() const;

    bool is_valid_use(DocumentNode* node) const;

    void on_use_changed(BrushStyle* new_use, BrushStyle* old_use);

    void on_update_style();

signals:
    void use_changed(BrushStyle* new_use);
    void use_changed_from(BrushStyle* old_use, BrushStyle* new_use);
};

} // namespace glaxnimate::model
