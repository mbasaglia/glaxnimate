#pragma once

#include <QObject>

#include "math/bezier/bezier.hpp"
#include "model/shapes/image.hpp"
#include "model/shapes/group.hpp"
#include "utils/trace.hpp"

namespace glaxnimate::utils::trace {

class TraceWrapper : public QObject
{
    Q_OBJECT

public:
    struct TraceResult
    {
        QColor color;
        math::bezier::MultiBezier bezier;
        std::vector<QRectF> rects;
    };

    explicit TraceWrapper(model::Image* image);
    ~TraceWrapper();

    void trace_mono(const QColor& color, bool inverted, int alpha_threshold, std::vector<TraceResult>& result);
    void trace_exact(const std::vector<QRgb>& colors, int tolerance, std::vector<TraceResult>& result);
    void trace_closest(const std::vector<QRgb>& colors, std::vector<TraceResult>& result);
    void trace_pixel(std::vector<TraceResult>& result);

    model::Group* apply(std::vector<TraceResult>& result, qreal stroke_width);

    QSize size() const;

    TraceOptions& options();
    const QImage& image() const;

signals:
    void progress_max_changed(int max);
    void progress_changed(int value);

private:
    class Private;
    std::unique_ptr<Private> d;

};

} // namespace glaxnimate::utils::trace
