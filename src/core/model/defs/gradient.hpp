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

class Gradient : public ObjectBase<Gradient, BrushStyle>
{
    GLAXNIMATE_OBJECT

public:
    enum Type
    {
        Linear = 1,
        Radial = 2
    };

    Q_ENUM(Type)

    GLAXNIMATE_PROPERTY_REFERENCE(GradientColors, colors, &Gradient::valid_refs, &Gradient::is_valid_ref, &Gradient::on_ref_changed)
    GLAXNIMATE_PROPERTY(Type, type, Linear, {}, {}, PropertyTraits::Visual)

    GLAXNIMATE_ANIMATABLE(QPointF, start_point, {})
    GLAXNIMATE_ANIMATABLE(QPointF, end_point, {})

    GLAXNIMATE_ANIMATABLE(QPointF, highlight, {})

public:
    using Ctor::Ctor;

    QString type_name_human() const override;
    QBrush brush_style(FrameTime t) const override;

    Q_INVOKABLE qreal radius(FrameTime t) const;

    static QString gradient_type_name(Type t);

private:

    std::vector<ReferenceTarget*> valid_refs() const;
    bool is_valid_ref(ReferenceTarget* node) const;

    void on_ref_changed(GradientColors* new_ref, GradientColors* old_ref);
    void on_ref_visual_changed();


    void fill_icon(QPixmap& icon) const override;

    void on_property_changed(const BaseProperty* prop, const QVariant& value) override;
};

} // namespace model
