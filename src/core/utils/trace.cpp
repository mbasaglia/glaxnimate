#include "trace.hpp"

#include "potracelib.h"


class utils::trace::TraceOptions::Private
{
public:
    potrace_param_s* params;
    static const constexpr qreal alphamax_max = 1.3334;
};

utils::trace::TraceOptions::TraceOptions()
    : d(std::make_unique<Private>())
{
    d->params = potrace_param_default();
}

utils::trace::TraceOptions::~TraceOptions()
{
    potrace_param_free(d->params);
}

int utils::trace::TraceOptions::min_area() const
{
    return d->params->turdsize;
}

void utils::trace::TraceOptions::set_min_area(int min_area)
{
    d->params->turdsize = min_area;
}

qreal utils::trace::TraceOptions::smoothness() const
{
    return d->params->alphamax / Private::alphamax_max;
}

void utils::trace::TraceOptions::set_smoothness(qreal smoothness)
{
    d->params->alphamax = smoothness * Private::alphamax_max;
}


class utils::trace::Tracer::Private
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

    static void progress_callback(double progress, void* privdata)
    {
        reinterpret_cast<Tracer*>(privdata)->progress(progress);
    }

    int get_bit_alpha(const uchar* pixel) const noexcept
    {
        return pixel[3] >= target_alpha;
    }

    int get_bit_alpha_neg(const uchar* pixel) const noexcept
    {
        return pixel[3] < target_alpha;
    }

    int get_bit_color(const uchar* pixel) const noexcept
    {
        return qRgba(pixel[0], pixel[1], pixel[2], pixel[3]) == target_color;
    }

    int get_bit_index(const uchar* pixel) const noexcept
    {
        return *pixel == target_color;
    }

    using callback_type = int (Private::*)(const uchar*) const noexcept;
    callback_type callback = &Private::get_bit_alpha;
    int target_alpha = 128;
    QRgb target_color;
    QImage image;
    potrace_param_s params;
};

utils::trace::Tracer::Tracer(const QImage& image, const TraceOptions& options)
    : d(std::make_unique<Private>())
{
    d->image = image;
    d->params = *options.d->params;
    d->params.progress = {
        &Private::progress_callback,
        this,
        0,
        100,
        1
    };
}

utils::trace::Tracer::~Tracer() = default;

bool utils::trace::Tracer::trace(math::bezier::MultiBezier& mbez)
{

    QImage::Format target_format = d->callback == &Private::get_bit_index ? QImage::Format_Indexed8 : QImage::Format_RGBA8888;
    if ( d->image.format() != target_format )
        d->image = d->image.convertToFormat(target_format);

    int line_len = d->image.width() / sizeof(potrace_word);
    if ( d->image.width() % sizeof(potrace_word) )
        line_len += 1;

    static constexpr int N = sizeof(potrace_word) * CHAR_BIT;
    const int x_off = d->callback == &Private::get_bit_index ? 1 : 4;
    std::vector<potrace_word> data(line_len * d->image.height(), 0);
    for ( int y = 0, h = d->image.height(), w = d->image.width(); y < h; y++ )
    {
        auto line = d->image.constScanLine(y);
        for ( int x = 0; x < w; x++ )
        {
            (data.data() + y*line_len)[x/N] |= (1ul << (N-1-x%N)) * (d.get()->*d->callback)(line+x*x_off);
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

QString utils::trace::Tracer::potrace_version()
{
    return ::potrace_version();
}

void utils::trace::Tracer::set_progress_range(double min, double max)
{
    d->params.progress.min = min;
    d->params.progress.max = max;
}

void utils::trace::Tracer::set_target_alpha(int threshold, bool invert)
{
    d->target_alpha = threshold;
    d->callback = invert ? &Private::get_bit_alpha_neg : &Private::get_bit_alpha;
}

void utils::trace::Tracer::set_target_color(const QColor& color)
{
    d->target_color = color.rgba();
    d->callback = &Private::get_bit_color;
}

void utils::trace::Tracer::set_target_index(uchar index)
{
    d->target_color = index;
    d->callback = &Private::get_bit_index;
}
