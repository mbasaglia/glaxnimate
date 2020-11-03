#pragma once
#include "model/defs/brush_style.hpp"
#include "model/animation/animatable.hpp"
#include "math/vector.hpp"

namespace math {

template<>
QGradientStops lerp<QGradientStops>(const QGradientStops& a, const QGradientStops& b, double factor);

} // namespace math

namespace model {

class GradientColors : public Asset
{
    GLAXNIMATE_OBJECT(GradientColors)

    GLAXNIMATE_ANIMATABLE(QGradientStops, colors, {}, &GradientColors::colors_changed)

public:
    using Asset::Asset;

    QIcon reftarget_icon() const override;
    QString type_name_human() const override;

    bool remove_if_unused(bool clean_lists) override;

    Q_INVOKABLE void split_segment(int segment_index, float factor = 0.5, const QColor& new_color = {});
    Q_INVOKABLE void remove_stop(int index);

signals:
    void colors_changed(const QGradientStops&);
};

class Gradient : public BrushStyle
{
    GLAXNIMATE_OBJECT(Gradient)

public:
    enum Type
    {
        Linear = 1,
        Radial = 2
    };

    Q_ENUM(Type)

    GLAXNIMATE_PROPERTY_REFERENCE(model::GradientColors, colors, &Gradient::valid_refs, &Gradient::is_valid_ref, &Gradient::on_ref_changed)
    GLAXNIMATE_PROPERTY(Type, type, Linear, {}, {}, PropertyTraits::Visual)

    GLAXNIMATE_ANIMATABLE(QPointF, start_point, {})
    GLAXNIMATE_ANIMATABLE(QPointF, end_point, {})

    GLAXNIMATE_ANIMATABLE(QPointF, highlight, {})

public:
    using BrushStyle::BrushStyle;

    QString type_name_human() const override;
    QBrush brush_style(FrameTime t) const override;

    Q_INVOKABLE qreal radius(FrameTime t) const;

    static QString gradient_type_name(Type t);

    bool remove_if_unused(bool clean_lists) override;

private:
    std::vector<ReferenceTarget*> valid_refs() const;
    bool is_valid_ref(ReferenceTarget* node) const;

    void on_ref_changed(GradientColors* new_ref, GradientColors* old_ref);
    void on_ref_visual_changed();


    void fill_icon(QPixmap& icon) const override;

    void on_property_changed(const BaseProperty* prop, const QVariant& value) override;
};

} // namespace model
