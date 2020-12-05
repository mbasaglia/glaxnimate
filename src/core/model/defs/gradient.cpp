#include "gradient.hpp"

#include <QPainter>

#include "model/document.hpp"
#include "model/defs/defs.hpp"

#include "command/object_list_commands.hpp"
#include "command/animation_commands.hpp"
#include "command/undo_macro_guard.hpp"

#include "utils/sort_gradient.hpp"


GLAXNIMATE_OBJECT_IMPL(model::GradientColors)
GLAXNIMATE_OBJECT_IMPL(model::Gradient)

template<>
std::optional<QGradientStops> model::detail::variant_cast<QGradientStops>(const QVariant& val)
{
    if ( !val.canConvert(qMetaTypeId<QGradientStops>()) )
    {
        if ( val.canConvert(QMetaType::QVariantList) )
        {
            QGradientStops stops;
            for ( auto stop : val.toList() )
            {
                if ( stop.canConvert<QGradientStop>() )
                {
                    stops.push_back(stop.value<QGradientStop>());
                }
                else if ( stop.canConvert(QMetaType::QVariantList) )
                {
                    auto sl = stop.toList();
                    if ( sl.size() == 2 && sl[0].canConvert(QMetaType::Double) && sl[1].canConvert(QMetaType::QColor) )
                        stops.push_back({sl[0].toDouble(), sl[1].value<QColor>()});
                }
            }
            return stops;
        }
        return {};
    }

    QVariant converted = val;
    if ( !converted.convert(qMetaTypeId<QGradientStops>()) )
        return {};
    return converted.value<QGradientStops>();
}


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

bool model::GradientColors::remove_if_unused(bool clean_lists)
{
    if ( clean_lists && users().empty() )
    {
        document()->push_command(new command::RemoveObject(
            this,
            &document()->defs()->gradient_colors
        ));
        return true;
    }

    return false;
}

static QVariant split_gradient(QGradientStops colors, int index, float factor, const QColor& new_color)
{
    int before = index;
    int after = index+1;

    if ( index >= colors.size() )
    {
        before = colors.size() - 2;
        after = colors.size() - 1;
    }

    colors.push_back({
        math::lerp(colors[before].first, colors[after].first, factor),
        new_color.isValid() ? new_color : math::lerp(colors[before].second, colors[after].second, 0.5)
    });

    utils::sort_gradient(colors);
    return QVariant::fromValue(colors);
}

void model::GradientColors::split_segment(int segment_index, float factor, const QColor& new_color)
{
    command::UndoMacroGuard guard(tr("Add color to %1").arg(name.get()), document());
    if ( segment_index < 0 )
        segment_index = 0;

    if ( !colors.animated() )
    {
        colors.set_undoable(split_gradient(colors.get(), segment_index, factor, new_color));
    }
    else
    {
        for ( const auto& kf : colors )
            document()->push_command(new command::SetKeyframe(
                &colors, kf.time(), split_gradient(kf.get(), segment_index, factor, new_color), true
            ));
    }
}

void model::GradientColors::remove_stop(int index)
{
    command::UndoMacroGuard guard(tr("Remove color from %1").arg(name.get()), document());

    if ( index < 0 )
        index = 0;

    if ( !colors.animated() )
    {
        auto stops = colors.get();
        stops.erase(std::min(stops.begin() + index, stops.end()));
        colors.set_undoable(QVariant::fromValue(stops));
    }
    else
    {
        for ( const auto& kf : colors )
        {
            auto stops = kf.get();
            stops.erase(std::min(stops.begin() + index, stops.end()));

            document()->push_command(new command::SetKeyframe(
                &colors, kf.time(), QVariant::fromValue(stops), true
            ));
        }
    }
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
        disconnect(old_ref, &GradientColors::colors_changed, this, &Gradient::on_ref_visual_changed);

    if ( new_ref )
    {
        connect(new_ref, &GradientColors::colors_changed, this, &Gradient::on_ref_visual_changed);
    }
    else
    {
        detach();
    }

    colors_changed_from(old_ref, new_ref);
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

QString model::Gradient::gradient_type_name(GradientType t)
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

bool model::Gradient::remove_if_unused(bool)
{
    if ( users().empty() )
    {
        colors.set_undoable(QVariant::fromValue((model::GradientColors*)nullptr));
        document()->push_command(new command::RemoveObject(
            this,
            &document()->defs()->gradients
        ));
        return true;
    }
    return false;
}
