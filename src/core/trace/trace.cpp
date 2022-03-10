#include "trace.hpp"

#include "potracelib.h"

#include "utils/color.hpp"

using namespace glaxnimate;


class trace::TraceOptions::Private
{
public:
    potrace_param_s* params;
    static const constexpr qreal alphamax_max = 1.3334;
};

trace::TraceOptions::TraceOptions()
    : d(std::make_unique<Private>())
{
    d->params = potrace_param_default();
}

trace::TraceOptions::~TraceOptions()
{
    potrace_param_free(d->params);
}

int trace::TraceOptions::min_area() const
{
    return d->params->turdsize;
}

void trace::TraceOptions::set_min_area(int min_area)
{
    d->params->turdsize = min_area;
}

qreal trace::TraceOptions::smoothness() const
{
    return d->params->alphamax / Private::alphamax_max;
}

void trace::TraceOptions::set_smoothness(qreal smoothness)
{
    d->params->alphamax = smoothness * Private::alphamax_max;
}

class trace::Tracer::Private
{
public:
    struct CurveWrapper
    {
        potrace_curve_s* curve;

        QPointF point(int index, int off) const
        {
            const auto& p = curve->c[index][off];
            return {p.x, p.y};
        }

        void to_bezier(math::bezier::MultiBezier& mbez) const
        {
            if ( curve->n < 2 )
                return;

            mbez.move_to(point(curve->n-1, 2));
            for ( int i = 0; i < curve->n - 1; i++ )
            {
                if ( curve->tag[i] == POTRACE_CURVETO )
                {
                    mbez.cubic_to(point(i, 0), point(i, 1), point(i, 2));
                }
                else
                {
                    mbez.line_to(point(i, 1));
                    mbez.line_to(point(i, 2));
                }
            }

            if ( curve->tag[curve->n - 1] == POTRACE_CURVETO )
            {
                mbez.beziers().back().points().back().tan_out = point(curve->n - 1, 0);
                mbez.beziers().back()[0].tan_in = point(curve->n - 1, 1);
            }
            else
            {
                mbez.line_to(point(curve->n - 1, 1));
            }

            mbez.close();

        }
    };

    Private(const SegmentedImage& img)
    : image(img)
    {}

    static void progress_callback(double progress, void* privdata)
    {
        reinterpret_cast<Tracer*>(privdata)->progress(progress);
    }

    int get_bit_color(Cluster::id_type pixel) const
    {
        return image.cluster(pixel)->color == target_color;
    }

    int get_bit_color_tolerance(Cluster::id_type pixel) const
    {
        return utils::color::rgba_distance_squared(target_color, image.cluster(pixel)->color) <= target_tolerance;
    }

    int get_bit_index(Cluster::id_type pixel) const
    {
        return pixel == target_index;
    }

    using callback_type = int (Private::*)(Cluster::id_type) const;
    callback_type callback = &Private::get_bit_index;
    Cluster::id_type target_index;
    QRgb target_color;
    qint32 target_tolerance = 0;
    SegmentedImage image;
    potrace_param_s params;
};

trace::Tracer::Tracer(const SegmentedImage& image, const TraceOptions& options)
    : d(std::make_unique<Private>(image))
{
    d->params = *options.d->params;
    d->params.progress = {
        &Private::progress_callback,
        this,
        0,
        100,
        1
    };
}

trace::Tracer::~Tracer() = default;

bool trace::Tracer::trace(math::bezier::MultiBezier& mbez)
{
    int line_len = d->image.width() / sizeof(potrace_word);
    if ( d->image.width() % sizeof(potrace_word) )
        line_len += 1;

    static constexpr int N = sizeof(potrace_word) * CHAR_BIT;
    std::vector<potrace_word> data(line_len * d->image.height(), 0);
    for ( int y = 0; y < d->image.height(); y++ )
    {
        for ( int x = 0; x < d->image.width(); x++ )
        {
            (data.data() + y*line_len)[x/N] |= (1ul << (N-1-x%N)) * (d.get()->*d->callback)(d->image.cluster_id(x, y));
        }
    }

    potrace_bitmap_s bitmap{
        d->image.width(),
        d->image.height(),
        line_len,
        data.data()
    };

    potrace_state_t *result = potrace_trace(&d->params, &bitmap);

    if ( result->status == POTRACE_STATUS_OK )
    {
        for ( auto path = result->plist; path; path = path->next )
        {
            Private::CurveWrapper{&path->curve}.to_bezier(mbez);
        }
        potrace_state_free(result);
        return true;
    }

    potrace_state_free(result);
    return false;
}

QString trace::Tracer::potrace_version()
{
    return ::potrace_version();
}

void trace::Tracer::set_progress_range(double min, double max)
{
    d->params.progress.min = min;
    d->params.progress.max = max;
}

void trace::Tracer::set_target_color(const QColor& color, qint32 tolerance)
{
    d->target_color = color.rgba();
    d->target_tolerance = tolerance;
    d->callback = tolerance > 0 ? &Private::get_bit_color_tolerance : &Private::get_bit_color;
}

void trace::Tracer::set_target_index(Cluster::id_type index)
{
    d->target_index = index;
    d->callback = &Private::get_bit_index;
}


struct PixelRect
{
    QRectF rect;
    QRgb color;
};

struct PixelTraceData
{
    std::map<int, PixelRect*> last_rects;
    QList<PixelRect> all_rects;

    void merge_up(PixelRect* last_rect, std::map<int, PixelRect*>& rects)
    {
        if ( !last_rect )
            return;

        auto yrect = get_rect(last_rect->rect.left());
        if ( yrect && yrect->rect.width() == last_rect->rect.width() && yrect->color == last_rect->color )
        {
            yrect->rect.setBottom(yrect->rect.bottom()+1);
            rects[last_rect->rect.left()] = yrect;

            for ( auto it = all_rects.begin(); it != all_rects.end(); ++it )
                if ( &*it == last_rect )
                {
                    all_rects.erase(it);
                    break;
                }
        }
    }

    PixelRect* get_rect(int left)
    {
        auto yrect = last_rects.find(left);
        if ( yrect == last_rects.end() )
            return nullptr;
        return &*yrect->second;
    }

    PixelRect* add_rect(QRgb color, int x, int y)
    {
        all_rects.push_back({QRectF(x, y, 1, 1), color});
        return &all_rects.back();
    }

};


std::map<QRgb, std::vector<QRectF> > trace::trace_pixels(const SegmentedImage& image)
{
    int w = image.width();
    int h = image.height();

    PixelTraceData data;

    for ( int y = 0; y < h; y++ )
    {
        std::map<int, PixelRect*> rects;
        QRgb last_color = 0;
        PixelRect* last_rect = nullptr;

        for ( int x = 0; x < w; x++ )
        {

            auto index = image.cluster_id(x, y);
            if ( index == 0 )
            {
                last_color = 0;
                last_rect = nullptr;
                continue;
            }

            QRgb colort = image.cluster(index)->color;

            auto yrect = data.get_rect(x);

            if ( colort == last_color )
            {
                last_rect->rect.setRight(last_rect->rect.right() + 1);
            }
            else if ( yrect && colort == yrect->color && yrect->rect.width() == 1 )
            {
                yrect->rect.setBottom(yrect->rect.bottom()+1);
                rects[x] = yrect;
                last_rect = nullptr;
                colort = 0;
            }
            else
            {
                data.merge_up(last_rect, rects);
                last_rect = data.add_rect(colort, x, y);
                rects.emplace(x, last_rect);
            }
            last_color = colort;
        }
        data.merge_up(last_rect, rects);
        std::swap(data.last_rects, rects);
    }

    std::map<QRgb, std::vector<QRectF> > traced;
    for ( const auto& r : data.all_rects )
        traced[r.color].push_back(r.rect);

    return traced;
}
