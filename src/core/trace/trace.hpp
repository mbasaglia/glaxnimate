#pragma once

#include <memory>

#include "math/bezier/bezier.hpp"
#include "segmentation.hpp"

namespace glaxnimate::trace {

class TraceOptions
{
public:
    TraceOptions();
    ~TraceOptions();

    qreal smoothness() const;
    void set_smoothness(qreal smoothness);

    int min_area() const;
    void set_min_area(int min_area);

private:
    friend class Tracer;
    class Private;
    std::unique_ptr<Private> d;
};

class Tracer : public QObject
{
    Q_OBJECT
public:
    Tracer(const SegmentedImage& image, const TraceOptions& options);
    ~Tracer();

    bool trace(math::bezier::MultiBezier& output);

    static QString potrace_version();

    void set_progress_range(double min, double max);

    void set_target_cluster(Cluster::id_type cluster);

signals:
    void progress(double value);

private:
    class Private;
    std::unique_ptr<Private> d;
};

std::map<QRgb, std::vector<QRectF>> trace_pixels(const SegmentedImage& image);

} // namespace glaxnimate::trace
