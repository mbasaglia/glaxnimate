#include "gradient.hpp"

#include <QPainter>

#include "model/document.hpp"
#include "model/assets/assets.hpp"

#include "command/object_list_commands.hpp"
#include "command/animation_commands.hpp"
#include "command/undo_macro_guard.hpp"

#include "utils/sort_gradient.hpp"


using namespace glaxnimate;


GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::GradientColors)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Gradient)

template<>
std::optional<QGradientStops> glaxnimate::model::detail::variant_cast<QGradientStops>(const QVariant& val)
{
    if ( !val.canConvert<QGradientStops>() )
    {
        if ( val.canConvert<QVariantList>() )
        {
            QGradientStops stops;
            for ( auto stop : val.toList() )
            {
                if ( stop.canConvert<QGradientStop>() )
                {
                    stops.push_back(stop.value<QGradientStop>());
                }
                else if ( stop.canConvert<QVariantList>() )
                {
                    auto sl = stop.toList();
                    if ( sl.size() == 2 && sl[0].canConvert<double>() && sl[1].canConvert<QColor>() )
                        stops.push_back({sl[0].toDouble(), sl[1].value<QColor>()});
                }
            }
            return stops;
        }
        return {};
    }

    QVariant converted = val;
#if QT_VERSION_MAJOR < 6
    if ( !converted.convert(qMetaTypeId<QGradientStops>()) )
#else
    if ( !converted.convert(QMetaType::fromType<QGradientStops>()) )
#endif
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

QString glaxnimate::model::GradientColors::type_name_human() const
{
    return tr("Gradient");
}


QIcon glaxnimate::model::GradientColors::instance_icon() const
{
    QPixmap icon(32, 32);
    QPainter p(&icon);
    QLinearGradient g(0, 0, icon.width(), 0);
    g.setStops(colors.get());
    p.fillRect(icon.rect(), g);
    return icon;
}

bool glaxnimate::model::GradientColors::remove_if_unused(bool clean_lists)
{
    if ( clean_lists && users().empty() )
    {
        document()->push_command(new command::RemoveObject(
            this,
            &document()->assets()->gradient_colors->values
        ));
        return true;
    }

    return false;
}

static QVariant split_gradient(QGradientStops colors, int index, float factor, const QColor& new_color)
{
    int before = index;
    int after = index+1;

    if ( after >= colors.size() )
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

void glaxnimate::model::GradientColors::split_segment(int segment_index, float factor, const QColor& new_color)
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

void glaxnimate::model::GradientColors::remove_stop(int index)
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

std::vector<glaxnimate::model::DocumentNode *> glaxnimate::model::Gradient::valid_refs() const
{
    return document()->assets()->gradient_colors->values.valid_reference_values(false);
}

bool glaxnimate::model::Gradient::is_valid_ref ( glaxnimate::model::DocumentNode* node ) const
{
    return document()->assets()->gradient_colors->values.is_valid_reference_value(node, true);
}

void glaxnimate::model::Gradient::on_ref_visual_changed()
{
    emit style_changed();
}

void glaxnimate::model::Gradient::on_ref_changed ( glaxnimate::model::GradientColors* new_ref, glaxnimate::model::GradientColors* old_ref )
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

QString glaxnimate::model::Gradient::type_name_human() const
{
    return tr("%1 Gradient").arg(gradient_type_name(type.get()));
}

QBrush glaxnimate::model::Gradient::brush_style ( glaxnimate::model::FrameTime t ) const
{
    if ( type.get() == Radial )
    {
        QRadialGradient g(start_point.get_at(t), radius(t), highlight.get_at(t));
        if ( colors.get() )
            g.setStops(colors->colors.get_at(t));
        g.setSpread(QGradient::PadSpread);
        return g;
    }
    else if ( type.get() == Conical )
    {
        auto start = start_point.get_at(t);
        auto end = end_point.get_at(t);
        auto angle = -math::rad2deg(math::atan2(end.y() - start.y(), end.x() - start.x()));
        QConicalGradient g(start, angle);
        if ( colors.get() )
            g.setStops(colors->colors.get_at(t));
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

QBrush glaxnimate::model::Gradient::constrained_brush_style(FrameTime t, const QRectF& bounds) const
{
    if ( type.get() == Radial )
    {
        QRadialGradient g(bounds.center(), bounds.width() / 2);
        if ( colors.get() )
            g.setStops(colors->colors.get_at(t));
        return g;
    }
    else if ( type.get() == Conical )
    {
        QConicalGradient g(bounds.center(), 02);
        if ( colors.get() )
            g.setStops(colors->colors.get_at(t));
        return g;
    }
    else
    {
        QLinearGradient g(bounds.topLeft(), bounds.topRight());
        if ( colors.get() )
            g.setStops(colors->colors.get_at(t));
        return g;
    }
}

void glaxnimate::model::Gradient::fill_icon(QPixmap& icon) const
{
    QPainter p(&icon);
    p.fillRect(icon.rect(), constrained_brush_style(time(), icon.rect()));
}

qreal glaxnimate::model::Gradient::radius(glaxnimate::model::FrameTime t) const
{
    return math::length(start_point.get_at(t) - end_point.get_at(t));
}

QString glaxnimate::model::Gradient::gradient_type_name(GradientType t)
{
    switch ( t )
    {
        case Linear:
            return tr("Linear");
        case Radial:
            return tr("Radial");
        case Conical:
            return tr("Conical");
    }

    return {};
}

void glaxnimate::model::Gradient::on_property_changed(const glaxnimate::model::BaseProperty*, const QVariant&)
{
    emit style_changed();
}

bool glaxnimate::model::Gradient::remove_if_unused(bool)
{
    if ( users().empty() )
    {
        colors.set_undoable(QVariant::fromValue((glaxnimate::model::GradientColors*)nullptr));
        document()->push_command(new command::RemoveObject(
            this,
            &document()->assets()->gradients->values
        ));
        return true;
    }
    return false;
}
