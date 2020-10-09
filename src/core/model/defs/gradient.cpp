#include "gradient.hpp"

#include <QPainter>

#include "model/document.hpp"
#include "model/defs/defs.hpp"

template<>
QGradientStops math::lerp<QGradientStops>(const QGradientStops& a, const QGradientStops& b, double factor)
{
    if ( a.size() != b.size() )
        return factor >= 1 ? b : a;

    QGradientStops mix;
    mix.reserve(a.size());
    for ( int i = 0; i < a.size(); i++ )
        mix.push_back({
            math::lerp(a[i].first, b[i].first, factor),
            math::lerp(a[i].second, b[i].second, factor)
        });

    return mix;
}

QString model::GradientColors::type_name_human() const
{
    return tr("Gradient");
}


QIcon model::GradientColors::reftarget_icon() const
{
    QPixmap icon(32, 32);
    QPainter p(&icon);
    QLinearGradient g(0, 0, icon.width(), 0);
    g.setStops(colors.get());
    p.fillRect(icon.rect(), g);
    return icon;
}

std::vector<model::ReferenceTarget *> model::Gradient::valid_refs() const
{
    return document()->defs()->gradient_colors.valid_reference_values(false);
}

bool model::Gradient::is_valid_ref ( model::ReferenceTarget* node ) const
{
    return document()->defs()->gradient_colors.is_valid_reference_value(node, true);
}

void model::Gradient::on_ref_visual_changed()
{
    emit property_changed(&colors, {});
}

void model::Gradient::on_ref_changed ( model::GradientColors* new_ref, model::GradientColors* old_ref )
{
    if ( old_ref )
    {
        old_ref->remove_user(&colors);
        disconnect(old_ref, &GradientColors::colors_changed, this, &Gradient::on_ref_visual_changed);
    }

    if ( new_ref )
    {
        new_ref->add_user(&colors);
        connect(new_ref, &GradientColors::colors_changed, this, &Gradient::on_ref_visual_changed);
    }
    else
    {
        detach();
    }
}

QString model::LinearGradient::type_name_human() const
{
    return tr("Linear Gradient");
}

QBrush model::LinearGradient::brush_style ( model::FrameTime t ) const
{
    QLinearGradient g(start_point.get_at(t), end_point.get_at(t));
    if ( colors.get() )
        g.setStops(colors->colors.get_at(t));
    g.setSpread(QGradient::PadSpread);
    return g;
}


void model::LinearGradient::fill_icon(QPixmap& icon) const
{
    QPainter p(&icon);
    QLinearGradient g(0, 0, icon.width(), 0);
    if ( colors.get() )
        g.setStops(colors->colors.get());
    p.fillRect(icon.rect(), g);
}

QString model::RadialGradient::type_name_human() const
{
    return tr("Radial Gradient");
}

QBrush model::RadialGradient::brush_style ( model::FrameTime t ) const
{
    QRadialGradient g(center.get_at(t), radius.get_at(t), highlight_center.get_at(t));
    if ( colors.get() )
        g.setStops(colors->colors.get_at(t));
    g.setSpread(QGradient::PadSpread);
    return g;
}

void model::RadialGradient::fill_icon ( QPixmap& icon ) const
{
    QPainter p(&icon);
    QRadialGradient g(icon.width() / 2, icon.height() / 2, icon.width() / 2);
    if ( colors.get() )
        g.setStops(colors->colors.get());
    p.fillRect(icon.rect(), g);
}



