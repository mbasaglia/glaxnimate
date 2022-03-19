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
