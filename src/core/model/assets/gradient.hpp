/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "model/assets/brush_style.hpp"
#include "model/animation/animatable.hpp"
#include "math/vector.hpp"

namespace glaxnimate::math {

template<>
QGradientStops lerp<QGradientStops>(const QGradientStops& a, const QGradientStops& b, double factor);

} // namespace glaxnimate::math

namespace glaxnimate::model {


namespace detail {

template<>
std::optional<QGradientStops> variant_cast<QGradientStops>(const QVariant& val);

} // namespace detail


class GradientColors : public Asset
{
    GLAXNIMATE_OBJECT(GradientColors)

    GLAXNIMATE_ANIMATABLE(QGradientStops, colors, {}, &GradientColors::colors_changed)

public:
    using Asset::Asset;

    QIcon instance_icon() const override;
    QString type_name_human() const override;

    bool remove_if_unused(bool clean_lists) override;

    Q_INVOKABLE void split_segment(int segment_index, float factor = 0.5, const QColor& new_color = {});
    Q_INVOKABLE void remove_stop(int index);

Q_SIGNALS:
    void colors_changed(const QGradientStops&);
};

class Gradient : public BrushStyle
{
    GLAXNIMATE_OBJECT(Gradient)

public:
    enum GradientType
    {
        Linear = 1,
        Radial = 2,
        Conical = 3
    };

    Q_ENUM(GradientType)

    GLAXNIMATE_PROPERTY_REFERENCE(GradientColors, colors, &Gradient::valid_refs, &Gradient::is_valid_ref, &Gradient::on_ref_changed)
    GLAXNIMATE_PROPERTY(GradientType, type, Linear, {}, {}, PropertyTraits::Visual)

    GLAXNIMATE_ANIMATABLE(QPointF, start_point, {})
    GLAXNIMATE_ANIMATABLE(QPointF, end_point, {})

    GLAXNIMATE_ANIMATABLE(QPointF, highlight, {})

public:
    using BrushStyle::BrushStyle;

    QString type_name_human() const override;
    QBrush brush_style(FrameTime t) const override;
    QBrush constrained_brush_style(FrameTime t, const QRectF& bounds) const override;

    Q_INVOKABLE qreal radius(double t) const;

    static QString gradient_type_name(GradientType t);

    bool remove_if_unused(bool clean_lists) override;

private:
    std::vector<DocumentNode*> valid_refs() const;
    bool is_valid_ref(DocumentNode* node) const;

    void on_ref_changed(GradientColors* new_ref, GradientColors* old_ref);
    void on_ref_visual_changed();


    void fill_icon(QPixmap& icon) const override;

    void on_property_changed(const BaseProperty* prop, const QVariant& value) override;

Q_SIGNALS:
    void colors_changed_from(GradientColors* old_use, GradientColors* new_use);
};

} // namespace glaxnimate::model
