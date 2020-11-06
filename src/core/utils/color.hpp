#pragma once
#include <QColor>


namespace utils::color {

inline constexpr qint32 rgba_distance_squared(QRgb c1, qint32 r, qint32 g, qint32 b, qint32 a) noexcept
{
    r -= qRed(c1);
    g -= qGreen(c1);
    b -= qBlue(c1);
    a -= qAlpha(c1);
    return r*r + g*g + b*b + a*a;
}

} // namespace utils::color
