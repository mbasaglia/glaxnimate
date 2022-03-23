#include "trace_wrapper.hpp"
#include "glaxnimate/trace/quantize.hpp"
#include "glaxnimate/core/model/document.hpp"
#include "glaxnimate/core/model/shapes/stroke.hpp"
#include "glaxnimate/core/model/shapes/fill.hpp"
#include "glaxnimate/core/model/shapes/path.hpp"
#include "glaxnimate/core/model/shapes/rect.hpp"
#include "glaxnimate/core/command/object_list_commands.hpp"

class glaxnimate::trace::TraceWrapper::Private
{
public:
    QImage source_image;
    trace::TraceOptions options;
    trace::SegmentedImage segmented{0, 0};
    trace::SegmentedImage segmented_cluster_merge{0, 0};
    trace::SegmentedImage segmented_quant{0, 0};
    BrushData cluster_merge_output;
    model::Image* image = nullptr;
    model::Document* document;
    QString name;

    void set_image(const QImage& image)
    {
        if ( image.format() != QImage::Format_ARGB32 )
            source_image = image.convertToFormat(QImage::Format_ARGB32);
        else
            source_image = image;

        segmented = segment(source_image);
        segmented.erase_if([](const Cluster& cluster){ return qAlpha(cluster.color) == 0; });
        segmented_cluster_merge = {0, 0};
        segmented_quant = {0, 0};
    }

    void result_to_shapes(model::ShapeListProperty& prop, const TraceResult& result, qreal stroke_width)
    {
        model::BrushStyle* style = nullptr;
        if ( result.gradient.stops.size() != 0 )
        {
            auto colors = document->assets()->add_gradient_colors();
            QGradientStops stops;
            stops.reserve(result.gradient.stops.size());
            for ( const auto& p : result.gradient.stops )
                stops.push_back({p.first, p.second.rgb()});
            colors->colors.set(stops);
            auto gradient = document->assets()->add_gradient();
            gradient->colors.set(colors);
            gradient->start_point.set(QPointF(result.gradient.p1.x, result.gradient.p1.y));
            gradient->end_point.set(QPointF(result.gradient.p2.x, result.gradient.p2.y));
            gradient->type.set(model::Gradient::Linear);
            style = gradient;
        }

        auto fill = std::make_unique<model::Fill>(document);
        fill->color.set(result.color);
        fill->use.set(style);
        prop.insert(std::move(fill));

        if ( stroke_width > 0 )
        {
            auto stroke = std::make_unique<model::Stroke>(document);
            stroke->color.set(result.color);
            stroke->width.set(stroke_width);
            stroke->use.set(style);
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

    trace::SegmentedImage& traceable()
    {
        if ( segmented_quant.size() != 0 )
            return segmented_quant;
        return segmented;
    }
};

glaxnimate::trace::TraceWrapper::TraceWrapper(model::Image* image)
    : TraceWrapper(image->document(), image->image->pixmap().toImage(), image->object_name())
{
    d->image = image;

}

glaxnimate::trace::TraceWrapper::TraceWrapper(model::Document* document, const QImage& image, const QString& name)
    : d(std::make_unique<Private>())
{
    d->document = document;
    d->name = name;
    d->set_image(image);
}


glaxnimate::trace::TraceWrapper::~TraceWrapper() = default;

QSize glaxnimate::trace::TraceWrapper::size() const
{
    return d->source_image.size();
}

glaxnimate::trace::TraceOptions & glaxnimate::trace::TraceWrapper::options()
{
    return d->options;
}

void glaxnimate::trace::TraceWrapper::trace_mono(
    const QColor& color, bool inverted, int alpha_threshold, std::vector<TraceResult>& result)
{
    result.emplace_back();
    emit progress_max_changed(100);
    result.back().color = color;
    auto mono = d->segmented;
    auto cluster = mono.mono([inverted, alpha_threshold](const Cluster& cluster){
        if ( inverted )
            return qAlpha(cluster.color) < alpha_threshold;
        return qAlpha(cluster.color) > alpha_threshold;
    });
    trace::Tracer tracer(d->segmented, d->options);
    tracer.set_target_cluster(cluster->id);
    connect(&tracer, &trace::Tracer::progress, this, &TraceWrapper::progress_changed);
    tracer.set_progress_range(0, 100);
    tracer.trace(result.back().bezier);
}

void glaxnimate::trace::TraceWrapper::trace_exact(
    const std::vector<QRgb>& colors, int tolerance, std::vector<TraceResult>& result
)
{
    result.reserve(result.size() + colors.size());
    emit progress_max_changed(100 * colors.size());
    int progress_index = 0;
    for ( QColor color : colors )
    {
        result.emplace_back();
        result.back().color = color;
        auto seg = d->segmented;
        Cluster* mono_cluster;
        if ( tolerance == 0 )
            mono_cluster = seg.mono([color=color.rgba()](const Cluster& c){
                return c.color == color;
            });
        else
            mono_cluster = seg.mono([tolerance, color=color.rgba()](const Cluster& c){
                return rgba_distance_squared(c.color, color) <= tolerance;
            });
        trace::Tracer tracer(d->segmented, d->options);
        tracer.set_target_cluster(mono_cluster->id);
        tracer.set_progress_range(100 * progress_index, 100 * (progress_index+1));
        tracer.trace(result.back().bezier);
        ++progress_index;
    }
}

void glaxnimate::trace::TraceWrapper::trace_closest(
    const BrushData& colors, std::vector<TraceResult>& result)
{
    const auto& converted = d->traceable();
    trace::Tracer tracer(converted, d->options);
    result.reserve(result.size() + converted.size());
    emit progress_max_changed(100 * converted.size());

    int i = 0;
    for ( const auto& cluster: converted )
    {
        tracer.set_target_cluster(cluster.id);
        tracer.set_progress_range(100 * i, 100 * (i+1));
        result.emplace_back();
        result.back().color = cluster.color;
        result.back().gradient = colors.gradient(cluster.id);
        tracer.trace(result.back().bezier);
        i++;
    }
}

void glaxnimate::trace::TraceWrapper::trace_pixel(std::vector<TraceResult>& result)
{
    auto pixdata = trace::trace_pixels(d->segmented);
    result.reserve(pixdata.size());
    for ( const auto& p : pixdata )
        result.push_back({p.first, {}, {}, p.second});
}

glaxnimate::model::Group* glaxnimate::trace::TraceWrapper::apply(
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
            &d->document->main()->shapes, std::move(layer)
        ));
    }

//     created->recursive_rename();
    return created;
}

const QImage & glaxnimate::trace::TraceWrapper::image() const
{
    return d->source_image;
}

const glaxnimate::trace::BrushData& glaxnimate::trace::TraceWrapper::cluster_merge_colors() const
{
    if ( d->cluster_merge_output.colors.empty() )
    {
        d->segmented_cluster_merge = d->segmented;
        d->cluster_merge_output = trace::cluster_merge(d->segmented_cluster_merge, 256);
    }
    return d->cluster_merge_output;
}

glaxnimate::trace::TraceWrapper::Preset
    glaxnimate::trace::TraceWrapper::preset_suggestion() const
{
    int w = d->source_image.width();
    int h = d->source_image.height();
    if ( w > 1024 || h > 1024 )
        return Preset::ComplexPreset;

    auto color_count = d->segmented.histogram().size();
    if ( w < 128 && h < 128 && color_count < 128 )
        return Preset::PixelPreset;

    color_count = cluster_merge_colors().colors.size();

    if ( w < 1024 && h < 1024 && color_count < 32 )
        return Preset::FlatPreset;
    else
        return Preset::ComplexPreset;
}



glaxnimate::trace::BrushData glaxnimate::trace::TraceWrapper::trace_preset(
    Preset preset, int complex_posterization, std::vector<TraceResult>& result
)
{
    d->options.set_min_area(16);
    d->options.set_smoothness(0.75);
    BrushData data;
    switch ( preset )
    {
        case trace::TraceWrapper::ComplexPreset:
            d->segmented_quant = d->segmented;
            data.colors = trace::octree(ColorFrequencies(d->segmented_quant), complex_posterization);
            d->segmented_quant.quantize(data.colors);
            trace_closest(data, result);
            d->segmented_quant = {0, 0};
            break;
        case trace::TraceWrapper::FlatPreset:
            data = cluster_merge_colors();
            d->segmented_quant = d->segmented_cluster_merge;
            trace_closest(data, result);
            d->segmented_quant = {0, 0};
            break;
        case trace::TraceWrapper::PixelPreset:
            trace_pixel(result);
            for ( const auto& r : result )
                data.colors.push_back(r.color.rgba());
            break;
    }
    return data;
}

const glaxnimate::trace::SegmentedImage & glaxnimate::trace::TraceWrapper::segmented_image() const
{
    return d->segmented;
}

glaxnimate::trace::SegmentedImage & glaxnimate::trace::TraceWrapper::segmented_image()
{
    return d->segmented;
}
