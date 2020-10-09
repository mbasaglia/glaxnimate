#pragma once
#include "model/defs/brush_style.hpp"
#include "model/animation/animatable.hpp"
#include "math/vector.hpp"

namespace math {

template<>
QGradientStops lerp<QGradientStops>(const QGradientStops& a, const QGradientStops& b, double factor);

} // namespace math

namespace model {

class GradientColors : public ObjectBase<GradientColors, Def>
{
    GLAXNIMATE_OBJECT

    GLAXNIMATE_ANIMATABLE(QGradientStops, colors, {}, &GradientColors::colors_changed)

public:
    using Ctor::Ctor;

    QIcon reftarget_icon() const override;
    QString type_name_human() const override;

signals:
    void colors_changed(const QGradientStops&);
};

class Gradient : public BrushStyle
{
    Q_OBJECT

    GLAXNIMATE_PROPERTY_REFERENCE(GradientColors, colors, &Gradient::valid_refs, &Gradient::is_valid_ref, &Gradient::on_ref_changed)

private:
    using BrushStyle::BrushStyle;

    std::vector<ReferenceTarget*> valid_refs() const;
    bool is_valid_ref(ReferenceTarget* node) const;

    void on_ref_changed(GradientColors* new_ref, GradientColors* old_ref);
    void on_ref_visual_changed();
};

class LinearGradient : public ObjectBase<LinearGradient, Gradient>
{
    GLAXNIMATE_OBJECT

    GLAXNIMATE_ANIMATABLE(QPointF, start_point, {})
    GLAXNIMATE_ANIMATABLE(QPointF, end_point, {})

public:
    using Ctor::Ctor;

    QString type_name_human() const override;
    QBrush brush_style(FrameTime t) const override;

private:
    void fill_icon(QPixmap& icon) const override;
};

class RadialGradient : public ObjectBase<LinearGradient, Gradient>
{
    GLAXNIMATE_OBJECT

    GLAXNIMATE_ANIMATABLE(QPointF, highlight_center, {})
    GLAXNIMATE_ANIMATABLE(QPointF, center, {})
    GLAXNIMATE_ANIMATABLE(float, radius, {})

public:
    using Ctor::Ctor;

    QString type_name_human() const override;
    QBrush brush_style(FrameTime t) const override;

private:
    void fill_icon(QPixmap& icon) const override;
};

} // namespace model
