#include "gradient.hpp"

#include <QPainter>

#include "model/document.hpp"
#include "model/defs/defs.hpp"
#include "command/object_list_commands.hpp"


GLAXNIMATE_OBJECT_IMPL(model::GradientColors)
GLAXNIMATE_OBJECT_IMPL(model::Gradient)


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
    emit style_changed();
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

QString model::Gradient::type_name_human() const
{
    return tr("%1 Gradient").arg(gradient_type_name(type.get()));
}

QBrush model::Gradient::brush_style ( model::FrameTime t ) const
{
    if ( type.get() == Radial )
    {
        QRadialGradient g(start_point.get_at(t), radius(t), highlight.get_at(t));
        if ( colors.get() )
            g.setStops(colors->colors.get_at(t));
        g.setSpread(QGradient::PadSpread);
        return g;
    }
    else
    {
        QLinearGradient g(start_point.get_at(t), end_point.get_at(t));
        if ( colors.get() )
            g.setStops(colors->colors.get_at(t));
        g.setSpread(QGradient::PadSpread);
        return g;
    }
}


void model::Gradient::fill_icon(QPixmap& icon) const
{
    QPainter p(&icon);

    if ( type.get() == Radial )
    {
        QRadialGradient g(icon.width() / 2, icon.height() / 2, icon.width() / 2);
        if ( colors.get() )
            g.setStops(colors->colors.get());
        p.fillRect(icon.rect(), g);
    }
    else
    {
        QLinearGradient g(0, 0, icon.width(), 0);
        if ( colors.get() )
            g.setStops(colors->colors.get());
        p.fillRect(icon.rect(), g);
    }
}

qreal model::Gradient::radius(model::FrameTime t) const
{
    return math::length(start_point.get_at(t) - end_point.get_at(t));
}

QString model::Gradient::gradient_type_name(Type t)
{
    switch ( t )
    {
        case Linear:
            return tr("Linear");
        case Radial:
            return tr("Radial");
    }

    return {};
}

void model::Gradient::on_property_changed(const model::BaseProperty*, const QVariant&)
{
    emit style_changed();
}

void model::Gradient::remove_if_unused()
{
    if ( users().empty() )
    {
        colors.set_undoable(QVariant::fromValue((model::GradientColors*)nullptr));
        document()->push_command(new command::RemoveObject(
            this,
            &document()->defs()->gradients
        ));
    }
}
