#pragma once

#include <algorithm>

#include <QGradient>

namespace utils {

inline bool gradient_stop_comparator(const QGradientStop& a, const QGradientStop& b) noexcept
{
    return a.first <= b.first;
}

inline void sort_gradient(QGradientStops& stops)
{
    std::sort(stops.begin(), stops.end(), &gradient_stop_comparator);
}

} // namespace utils
