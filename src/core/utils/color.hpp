#pragma once
#include <QColor>


namespace glaxnimate::utils::color {

inline constexpr qint32 rgba_distance_squared(QRgb c1, qint32 r, qint32 g, qint32 b, qint32 a) noexcept
{
    r -= qRed(c1);
    g -= qGreen(c1);
    b -= qBlue(c1);
    a -= qAlpha(c1);
    return r*r + g*g + b*b + a*a;
}


inline constexpr qint32 rgba_distance_squared(QRgb p1, QRgb p2) noexcept
{
    return rgba_distance_squared(p1, qRed(p2), qGreen(p2), qBlue(p2), qAlpha(p2));
}

} // namespace glaxnimate::utils::color
