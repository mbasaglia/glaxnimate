#pragma once

#include <QGradient>
#include <functional>

#include "color.hpp"
#include "segmentation.hpp"


namespace glaxnimate::trace {

/**
 * \brief Given a sequence of colors, it returns the gradient approximating the colors
 */
GradientStops gradient_stops(const std::vector<StructuredColor>& colors);

/**
 * \brief Finds the farthest point in the cluster from the center of its bounding box and get that angle
 * \return An angle in radians between 0 and pi
 */
qreal cluster_angle(SegmentedImage& image, Cluster::id_type id, const ImageCoord& origin);

/**
 * \brief Returns the intersections of a line with a rect
 * \param origin Point the line passes through
 * \param angle Angle in radians of the line
 * \param bounds Rectangle to intersect
 * \returns The two points where the line intersects the rect
 * \pre Intersection exists, usually you want \p origin to be within \p bounds
 */
std::pair<ImageCoord, ImageCoord> line_rect_intersection(const ImageCoord& origin, qreal angle, const ImageRect& bounds);

/**
 * \brief Returns the pixels in a line between two points
 * \see https://en.wikipedia.org/wiki/Bresenham's_line_algorithm
 * \pre p1.x <= p2.x
 */
std::vector<ImageCoord> line_pixels(const ImageCoord& p1, const ImageCoord& p2);

} // namespace glaxnimate::trace


namespace std {
    template<>
    struct hash<glaxnimate::trace::ImageCoord>
    {
        std::size_t operator()(const glaxnimate::trace::ImageCoord &p) const
        {
            std::size_t h1 = std::hash<int>{}(p.x);
            std::size_t h2 = std::hash<int>{}(p.y);
            return h1 ^ (h2 << 1);
        }
    };
}
