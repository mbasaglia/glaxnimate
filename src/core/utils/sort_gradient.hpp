/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <algorithm>

#include <QGradient>

namespace glaxnimate::utils {

inline bool gradient_stop_comparator(const QGradientStop& a, const QGradientStop& b) noexcept
{
    return a.first <= b.first;
}

inline void sort_gradient(QGradientStops& stops)
{
    std::sort(stops.begin(), stops.end(), &gradient_stop_comparator);
}

} // namespace glaxnimate::utils
