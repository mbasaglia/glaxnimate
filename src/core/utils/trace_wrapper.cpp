/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "trace_wrapper.hpp"
#include "utils/quantize.hpp"
#include "model/document.hpp"
#include "model/shapes/stroke.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/path.hpp"
#include "model/shapes/rect.hpp"
#include "command/object_list_commands.hpp"

class glaxnimate::utils::trace::TraceWrapper::Private
{
public:
    QImage source_image;
    utils::trace::TraceOptions options;
    std::vector<QRgb> eem_colors;
    model::Image* image = nullptr;
    model::Document* document;
    model::Composition* comp;
    QString name;

    void set_image(const QImage& image)
    {
        if ( image.format() != QImage::Format_RGBA8888 )
            source_image = image.convertToFormat(QImage::Format_RGBA8888);
        else
            source_image = image;
    }

    void result_to_shapes(model::ShapeListProperty& prop, const TraceResult& result, qreal stroke_width)
    {
        auto fill = std::make_unique<model::Fill>(document);
        fill->color.set(result.color);
        prop.insert(std::move(fill));

        if ( stroke_width > 0 )
        {
            auto stroke = std::make_unique<model::Stroke>(document);
            stroke->color.set(result.color);
            stroke->width.set(stroke_width);
            prop.insert(std::move(stroke));
        }

        for ( const auto& bez : result.bezier.beziers() )
        {
            auto path = std::make_unique<model::Path>(document);
            path->shape.set(bez);
            prop.insert(std::move(path));
        }

        for ( const auto& rect : result.rects )
        {
            auto shape = std::make_unique<model::Rect>(document);
            shape->position.set(rect.center());
            shape->size.set(rect.size());
            prop.insert(std::move(shape));
        }
    }

};

glaxnimate::utils::trace::TraceWrapper::TraceWrapper(model::Image* image)
    : TraceWrapper(image->owner_composition(), image->image->pixmap().toImage(), image->object_name())
{
    d->image = image;

}

glaxnimate::utils::trace::TraceWrapper::TraceWrapper(model::Composition* comp, const QImage& image, const QString& name)
    : d(std::make_unique<Private>())
{
    d->comp = comp;
    d->document = comp->document();
    d->name = name;
    d->set_image(image);
}


glaxnimate::utils::trace::TraceWrapper::~TraceWrapper() = default;

QSize glaxnimate::utils::trace::TraceWrapper::size() const
{
    return d->source_image.size();
}

glaxnimate::utils::trace::TraceOptions & glaxnimate::utils::trace::TraceWrapper::options()
{
    return d->options;
}

void glaxnimate::utils::trace::TraceWrapper::trace_mono(
    const QColor& color, bool inverted, int alpha_threshold, std::vector<TraceResult>& result)
{
    result.emplace_back();
    Q_EMIT progress_max_changed(100);
    result.back().color = color;
    utils::trace::Tracer tracer(d->source_image, d->options);
    tracer.set_target_alpha(alpha_threshold, inverted);
    connect(&tracer, &utils::trace::Tracer::progress, this, &TraceWrapper::progress_changed);
    tracer.set_progress_range(0, 100);
    tracer.trace(result.back().bezier);
}

void glaxnimate::utils::trace::TraceWrapper::trace_exact(
    const std::vector<QRgb>& colors, int tolerance, std::vector<TraceResult>& result
)
{
    result.reserve(result.size() + colors.size());
    Q_EMIT progress_max_changed(100 * colors.size());
    int progress_index = 0;
    for ( QColor color : colors )
    {
        result.emplace_back();
        result.back().color = color;
        utils::trace::Tracer tracer(d->source_image, d->options);
        tracer.set_target_color(color, tolerance * tolerance);
        tracer.set_progress_range(100 * progress_index, 100 * (progress_index+1));
        tracer.trace(result.back().bezier);
        ++progress_index;
    }
}

void glaxnimate::utils::trace::TraceWrapper::trace_closest(
    const std::vector<QRgb>& colors, std::vector<TraceResult>& result)
{
    Q_EMIT progress_max_changed(100 * colors.size());
    QImage converted = utils::quantize::quantize(d->source_image, colors);
    utils::trace::Tracer tracer(converted, d->options);
    result.reserve(result.size() + colors.size());

    for ( int i = 0; i < int(colors.size()); i++ )
    {
        tracer.set_target_index(i);
        tracer.set_progress_range(100 * i, 100 * (i+1));
        result.emplace_back();
        result.back().color = colors[i];
        tracer.trace(result.back().bezier);
    }
}

void glaxnimate::utils::trace::TraceWrapper::trace_pixel(std::vector<TraceResult>& result)
{
    auto pixdata = utils::trace::trace_pixels(d->source_image);
    result.reserve(pixdata.size());
    for ( const auto& p : pixdata )
        result.push_back({p.first, {}, p.second});
}

glaxnimate::model::Group* glaxnimate::utils::trace::TraceWrapper::apply(
    std::vector<TraceResult>& trace, qreal stroke_width
)
{
    auto layer = std::make_unique<model::Group>(d->document);
    auto created = layer.get();
    layer->name.set(tr("Traced %1").arg(d->name));

    if ( trace.size() == 1 )
    {
        d->result_to_shapes(layer->shapes, trace[0], stroke_width);
    }
    else
    {
        for ( const auto& result : trace )
        {
            auto group = std::make_unique<model::Group>(d->document);
            group->name.set(result.color.name());
            group->group_color.set(result.color);
            d->result_to_shapes(group->shapes, result, stroke_width);
            layer->shapes.insert(std::move(group));
        }
    }

    if ( d->image )
    {
        layer->transform->copy(d->image->transform.get());
        d->document->push_command(new command::AddObject<model::ShapeElement>(
            d->image->owner(), std::move(layer), d->image->position()+1
        ));
    }
    else
    {
        d->document->push_command(new command::AddObject<model::ShapeElement>(
            &d->comp->shapes, std::move(layer)
        ));
    }

//     created->recursive_rename();
    return created;
}

const QImage & glaxnimate::utils::trace::TraceWrapper::image() const
{
    return d->source_image;
}

const std::vector<QRgb>& glaxnimate::utils::trace::TraceWrapper::eem_colors() const
{
    if ( d->eem_colors.empty() )
        d->eem_colors = utils::quantize::edge_exclusion_modes(d->source_image, 256);
    return d->eem_colors;
}

glaxnimate::utils::trace::TraceWrapper::Preset
    glaxnimate::utils::trace::TraceWrapper::preset_suggestion() const
{
    int w = d->source_image.width();
    int h = d->source_image.height();
    if ( w > 1024 || h > 1024 )
        return Preset::ComplexPreset;

    auto color_count = utils::quantize::color_frequencies(d->source_image).size();
    if ( w < 128 && h < 128 && color_count < 128 )
        return Preset::PixelPreset;

    color_count = eem_colors().size();

    if ( w < 1024 && h < 1024 && color_count < 32 )
        return Preset::FlatPreset;
    else
        return Preset::ComplexPreset;
}



void glaxnimate::utils::trace::TraceWrapper::trace_preset(
    Preset preset, int complex_posterization, std::vector<QRgb> &colors, std::vector<TraceResult>& result
)
{
    d->options.set_min_area(16);
    d->options.set_smoothness(0.75);
    switch ( preset )
    {
        case utils::trace::TraceWrapper::ComplexPreset:
            colors = utils::quantize::octree(d->source_image, complex_posterization);
            trace_closest(colors, result);
            break;
        case utils::trace::TraceWrapper::FlatPreset:
            colors = eem_colors();
            trace_closest(colors, result);
            break;
        case utils::trace::TraceWrapper::PixelPreset:
            trace_pixel(result);
            break;
    }
}
