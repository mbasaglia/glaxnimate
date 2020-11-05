#pragma once

#include <QImage>
#include <memory>

#include "math/bezier/bezier.hpp"

namespace utils::trace {

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
    Tracer(const QImage& image, const TraceOptions& options);
    ~Tracer();

    bool trace(math::bezier::MultiBezier& output);

    static QString potrace_version();

    void set_progress_range(double min, double max);

    void set_target_alpha(int threshold, bool invert);
    void set_target_color(const QColor& color);
    void set_target_index(uchar index);

signals:
    void progress(double value);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace utils::trace
