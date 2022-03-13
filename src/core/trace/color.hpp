#pragma once

#include <vector>
#include <unordered_map>

#include <QColor>
#include <QGradient>

#include "math/vector.hpp"

namespace glaxnimate::trace {


using ColorFrequency = std::pair<QRgb, int>;

using Histogram = std::unordered_map<ColorFrequency::first_type, ColorFrequency::second_type>;


using ColorDistance = qint32;

/**
 * \brief Distance between two colors
 */
inline ColorDistance rgba_distance_squared(QRgb p1, QRgb p2)
{
    int r1 = qRed(p1);
    int g1 = qGreen(p1);
    int b1 = qBlue(p1);
    int a1 = qAlpha(p1);

    int r2 = qRed(p2);
    int g2 = qGreen(p2);
    int b2 = qBlue(p2);
    int a2 = qAlpha(p2);

    qint32 dr = r1 - r2;
    qint32 dg = g1 - g2;
    qint32 db = b1 - b2;
    qint32 da = a1 - a2;

    return dr * dr + dg * dg + db * db + da * da;
}

/**
 * \brief Returns the index in \p palette that is the closest to \p pixel
 */
qint32 closest_match(QRgb pixel, const std::vector<QRgb> &palette);


struct StructuredColor
{
    qint32 r;
    qint32 g;
    qint32 b;
    qint32 a;

    constexpr StructuredColor(QRgb rgb) noexcept
    : r(qRed(rgb)), g(qGreen(rgb)), b(qBlue(rgb)), a(qAlpha(rgb))
    {}

    constexpr StructuredColor() noexcept
    : r(0), g(0), b(0), a(0)
    {}

    constexpr StructuredColor(qint32 r, qint32 g, qint32 b, qint32 a = 255) noexcept
    : r(r), g(g), b(b), a(a)
    {}

    constexpr ColorDistance distance(const StructuredColor& oth) const noexcept
    {
        ColorDistance dr = r - oth.r;
        ColorDistance dg = g - oth.g;
        ColorDistance db = b - oth.b;
        ColorDistance da = a - oth.a;

        return dr * dr + dg * dg + db * db + da * da;
    }

    constexpr QRgb rgb() const noexcept
    {
        return qRgba(r, g, b, a);
    }

    constexpr void weighted_add(const StructuredColor& oth, int weight) noexcept
    {
        r += oth.r * weight;
        g += oth.g * weight;
        b += oth.b * weight;
        a += oth.a * weight;
    }

    constexpr StructuredColor lerp(const StructuredColor& oth, float factor) const noexcept
    {
        return {
            math::lerp(r, oth.r, factor),
            math::lerp(g, oth.g, factor),
            math::lerp(b, oth.b, factor),
            math::lerp(a, oth.a, factor),
        };
    }

    constexpr StructuredColor& operator+=(const StructuredColor& oth) noexcept
    {
        weighted_add(oth, 1);
        return *this;
    }

    constexpr StructuredColor mean(qreal total_weight) const noexcept
    {
        return {
            qRound(r / total_weight),
            qRound(g / total_weight),
            qRound(b / total_weight),
            qRound(a / total_weight)
        };
    }

    QColor qcolor() const
    {
        return QColor(r, g, b, a);
    }


    constexpr bool operator==(const StructuredColor& oth) const noexcept
    {
        return r == oth.r &&
               g == oth.g &&
               b == oth.b &&
               a == oth.a;
    }
};

/**
 * \brief Given a sequence of colors, it returns the gradient approximating the colors
 */
QGradientStops gradient_stops(const std::vector<StructuredColor>& colors);

} // namespace glaxnimate::trace
