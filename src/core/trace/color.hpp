#pragma once

#include <vector>
#include <unordered_map>

#include <QColor>

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
    quint8 r;
    quint8 g;
    quint8 b;
    quint8 a;

    constexpr StructuredColor(QRgb rgb) noexcept
    : r(qRed(rgb)), g(qGreen(rgb)), b(qBlue(rgb)), a(qAlpha(rgb))
    {}

    constexpr StructuredColor() noexcept
    : r(0), g(0), b(0), a(0)
    {}

    constexpr StructuredColor(quint8 r, quint8 g, quint8 b, quint8 a = 255) noexcept
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

    constexpr StructuredColor lerp(const StructuredColor& oth, float factor) const noexcept
    {
        return {
            math::lerp(r, oth.r, factor),
            math::lerp(g, oth.g, factor),
            math::lerp(b, oth.b, factor),
            math::lerp(a, oth.a, factor),
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

} // namespace glaxnimate::trace
