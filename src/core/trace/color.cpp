#include "color.hpp"


qint32 glaxnimate::trace::closest_match(QRgb pixel, const std::vector<QRgb> &palette)
{
    int idx = 0;
    qint32 current_distance = 255 * 255 * 3;
    for ( std::size_t i = 0; i < palette.size(); ++i)
    {
        int dist = rgba_distance_squared(pixel, palette[i]);
        if ( dist < current_distance )
        {
            current_distance = dist;
            idx = i;
        }
    }
    return idx;
}

QGradientStops glaxnimate::trace::gradient_stops(const std::vector<StructuredColor>& colors)
{
    struct ColorSlope
    {
        std::array<int, 4> deltas;

        ColorSlope(StructuredColor a, StructuredColor b)
            : deltas{
                b.r - a.r,
                b.g - a.g,
                b.b - a.b,
                b.a - a.a
            }
        {}

        bool operator!=(const ColorSlope& other) const
        {
            for ( int i = 0; i < 4; i++ )
                if ( qAbs(other.deltas[i] - deltas[i]) > 1 )
                    return true;
            return false;
        }
    };

    QGradientStops stops;
    stops.push_back({0, colors[0].qcolor()});
    if ( colors.size() > 2 )
    {
        ColorSlope prev_slope(colors[1], colors[2]);
        for ( std::size_t i = 1; i < colors.size() - 1; i++ )
        {
            ColorSlope next_slope(colors[i], colors[i+1]);
            if ( prev_slope != next_slope )
            {
                stops.push_back({i / float(colors.size() - 1), colors[i].qcolor()});
            }

            prev_slope = next_slope;
        }

        stops.push_back({1, colors.back().qcolor()});
    }

    return stops;
}
