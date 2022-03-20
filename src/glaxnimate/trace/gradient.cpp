#include "gradient.hpp"

#include "glaxnimate/math/math.hpp"


glaxnimate::trace::GradientStops glaxnimate::trace::gradient_stops(const std::vector<StructuredColor>& colors)
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

    GradientStops stops;
    stops.push_back({0, colors[0]});
    if ( colors.size() > 2 )
    {
        ColorSlope prev_slope(colors[1], colors[2]);
        for ( std::size_t i = 1; i < colors.size() - 1; i++ )
        {
            ColorSlope next_slope(colors[i], colors[i+1]);
            if ( prev_slope != next_slope )
            {
                stops.push_back({i / float(colors.size() - 1), colors[i]});
            }

            prev_slope = next_slope;
        }

        stops.push_back({1, colors.back()});
    }

    return stops;
}

qreal glaxnimate::trace::cluster_angle(SegmentedImage& image, Cluster::id_type id, const ImageCoord& origin)
{
    auto cluster = image.cluster(id);
    int max_dist = 0;
    int mx = 0;
    int my = 0;
    for ( auto i = cluster->index_start; i <= cluster->index_end; i++ )
    {
        if ( image.bitmap()[i] == id )
        {
            int y = i / image.width();
            int x = i % image.width();

            int dx = x - origin.x;
            int dy = y - origin.y;
            int dist = dx * dx + dy * dy;

            if ( dist > max_dist )
            {
                max_dist = dist;
                mx = dx;
                my = dy;
            }
        }
    }

    qreal angle = math::atan2(my, mx);
    // atan2 returns angle in [-pi, pi]
    if ( angle < 0 )
        angle += math::pi;

    return angle;
}

std::pair<glaxnimate::trace::ImageCoord, glaxnimate::trace::ImageCoord> glaxnimate::trace::line_rect_intersection(
    const glaxnimate::trace::ImageCoord& origin, qreal angle, const glaxnimate::trace::ImageRect& bounds
)
{
    if ( qFuzzyCompare(angle, math::pi) || qFuzzyIsNull(angle) )
    {
        return {{bounds.top_left.x, origin.y}, {bounds.bottom_right.x, origin.y}};
    }
    else if ( qFuzzyCompare(angle, math::pi/2) )
    {
        return {{origin.x, bounds.top_left.y}, {origin.x, bounds.bottom_right.y}};
    }

    // Find the slope of the line
    qreal m = math::tan(angle);

    // Line equation:
    // y - origin.y = m * ( x - origin.x )

    int y_at_min_x = m * (bounds.top_left.x - origin.x) + origin.y;
    int x_at_min_y = (bounds.top_left.y - origin.y) / m + origin.x;
    int y_at_max_x = m * (bounds.bottom_right.x - origin.x) + origin.y;
    int x_at_max_y = (bounds.bottom_right.y - origin.y) / m + origin.x;

    // find the leftmost point
    ImageCoord p1;
    if ( bounds.top_left.y <= y_at_min_x && y_at_min_x <= bounds.bottom_right.y )
        p1 = {bounds.top_left.x, y_at_min_x};
    else if ( x_at_min_y < x_at_max_y && bounds.top_left.x <= x_at_min_y && x_at_min_y <= bounds.bottom_right.x )
        p1 = {x_at_min_y, bounds.top_left.y};
    else
        p1 = {x_at_max_y, bounds.bottom_right.y};

    // find the rightmost point
    ImageCoord p2;
    if ( bounds.top_left.y <= y_at_max_x && y_at_max_x <= bounds.bottom_right.y )
        p2 = {bounds.bottom_right.x, y_at_max_x};
    else if ( x_at_min_y < x_at_max_y && bounds.top_left.x <= x_at_max_y && x_at_max_y <= bounds.bottom_right.x )
        p2 = {x_at_max_y, bounds.bottom_right.y};
    else
        p2 = {x_at_min_y, bounds.top_left.y};

    return {p1, p2};
}


std::vector<glaxnimate::trace::ImageCoord> glaxnimate::trace::line_pixels(const ImageCoord& p1, const ImageCoord& p2)
{
    int dx = p2.x - p1.x;
    int dy = -qAbs(p2.y - p1.y);
    int ysign = p2.y > p1.y ? 1 : -1;
    int error = dy + dx;
    int y = p1.y;
    int x = p1.x;

    std::vector<glaxnimate::trace::ImageCoord> result;
    result.reserve(dx);

    while ( x <= p2.x )
    {
        result.emplace_back(x, y);
        if ( x == p2.x && y == p2.y )
            break;
        int e2 = 2 * error;
        if ( e2 >= dy )
        {
            error += dy;
            x += 1;
        }
        if ( e2 <= dx )
        {
            error += dx;
            y += ysign;
        }
    }

    return result;
}
