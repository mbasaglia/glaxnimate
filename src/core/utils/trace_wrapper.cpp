#include "trace_wrapper.hpp"
#include "utils/quantize.hpp"
#include "model/shapes/stroke.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/path.hpp"
#include "model/shapes/rect.hpp"
#include "command/object_list_commands.hpp"

class glaxnimate::utils::trace::TraceWrapper::Private
{
public:
    model::Image* image;
    QImage source_image;
    utils::trace::TraceOptions options;


    void result_to_shapes(model::ShapeListProperty& prop, const TraceResult& result, qreal stroke_width)
    {
        auto fill = std::make_unique<model::Fill>(image->document());
        fill->color.set(result.color);
        prop.insert(std::move(fill));

        if ( stroke_width > 0 )
        {
            auto stroke = std::make_unique<model::Stroke>(image->document());
            stroke->color.set(result.color);
            stroke->width.set(stroke_width);
            prop.insert(std::move(stroke));
        }

        for ( const auto& bez : result.bezier.beziers() )
        {
            auto path = std::make_unique<model::Path>(image->document());
            path->shape.set(bez);
            prop.insert(std::move(path));
        }

        for ( const auto& rect : result.rects )
        {
            auto shape = std::make_unique<model::Rect>(image->document());
            shape->position.set(rect.center());
            shape->size.set(rect.size());
            prop.insert(std::move(shape));
        }
    }

};

glaxnimate::utils::trace::TraceWrapper::TraceWrapper(model::Image* image)
    : d(std::make_unique<Private>())
{
    d->image = image;
    d->source_image = image->image->pixmap().toImage();
    if ( d->source_image.format() != QImage::Format_RGBA8888 )
        d->source_image = d->source_image.convertToFormat(QImage::Format_RGBA8888);
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
    emit progress_max_changed(100);
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
    emit progress_max_changed(100 * colors.size());
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
    emit progress_max_changed(100 * colors.size());
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
    auto layer = std::make_unique<model::Group>(d->image->document());
    auto created = layer.get();
    layer->name.set(tr("Traced %1").arg(d->image->object_name()));
    layer->transform->copy(d->image->transform.get());

    if ( trace.size() == 1 )
    {
        d->result_to_shapes(layer->shapes, trace[0], stroke_width);
    }
    else
    {
        for ( const auto& result : trace )
        {
            auto group = std::make_unique<model::Group>(d->image->document());
            group->name.set(result.color.name());
            group->group_color.set(result.color);
            d->result_to_shapes(group->shapes, result, stroke_width);
            layer->shapes.insert(std::move(group));
        }
    }

    d->image->push_command(new command::AddObject<model::ShapeElement>(
        d->image->owner(), std::move(layer), d->image->position()+1
    ));

//     created->recursive_rename();
    return created;
}

const QImage & glaxnimate::utils::trace::TraceWrapper::image() const
{
    return d->source_image;
}
