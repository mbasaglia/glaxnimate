#pragma once

#include <QObject>

#include "glaxnimate/math/bezier/bezier.hpp"
#include "glaxnimate/core/model/shapes/image.hpp"
#include "glaxnimate/core/model/shapes/group.hpp"
#include "glaxnimate/trace/trace.hpp"
#include "glaxnimate/trace/quantize.hpp"

namespace glaxnimate::trace {

class TraceWrapper : public QObject
{
    Q_OBJECT

public:
    struct TraceResult
    {
        QColor color;
        Gradient gradient;
        math::bezier::MultiBezier bezier;
        std::vector<QRectF> rects;
    };

    enum Preset
    {
        ComplexPreset,
        FlatPreset,
        PixelPreset,
    };

    explicit TraceWrapper(model::Image* image);
    explicit TraceWrapper(model::Document* document, const QImage& image, const QString& name);
    ~TraceWrapper();

    void trace_mono(const QColor& color, bool inverted, int alpha_threshold, std::vector<TraceResult>& result);
    void trace_exact(const std::vector<QRgb>& colors, int tolerance, std::vector<TraceResult>& result);
    void trace_closest(const BrushData& colors, std::vector<TraceResult>& result);
    void trace_pixel(std::vector<TraceResult>& result);

    model::Group* apply(std::vector<TraceResult>& result, qreal stroke_width);

    QSize size() const;

    TraceOptions& options();
    const QImage& image() const;
    const SegmentedImage& segmented_image() const;
    SegmentedImage& segmented_image();

    const BrushData& cluster_merge_colors() const;

    Preset preset_suggestion() const;
    BrushData trace_preset(Preset preset, int complex_posterization, std::vector<TraceResult>& result);

signals:
    void progress_max_changed(int max);
    void progress_changed(int value);

private:
    class Private;
    std::unique_ptr<Private> d;

};

} // namespace glaxnimate::trace
