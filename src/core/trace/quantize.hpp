#pragma once

#include "segmentation.hpp"

namespace glaxnimate::trace {


/**
 * \brief Returns the \p k colors that appear most frequently in \p image.
 */
std::vector<QRgb> k_modes(SegmentedImage& image, int k);


enum KMeansMatch
{
    None,
    MostFrequent,
    Closest,
};

/**
 * \brief k-means Algorithm
 */
std::vector<QRgb> k_means(SegmentedImage& image, int k, int iterations, KMeansMatch match);


/**
 * \brief Octree Algorithm
 */
std::vector<QRgb> octree(SegmentedImage& image, int k);


/**
 * \brief Edge cutoff
 *
 * Selects the most frequent color, then excludes areas within 1 pixel of that color
 * Then it repeats until all the colors have been found
 *
 * \param image Image to get the colors for
 * \param max_colors Maximum number of colors
 * \param min_area A cluster must have at least min_area pixels to be selected
 */
std::vector<QRgb> edge_exclusion_modes(SegmentedImage& image, int max_colors, int min_area = 4);


std::vector<QRgb> cluster_merge(SegmentedImage& image, int max_colors, int min_area = 4, int min_color_distance = 16);

/**
 * \brief Counts pixel values and returns a list of [rgba, count] pairs
 * \param image             The image to analyze
 * \param alpha_threshold   Minimum alpha value [0-255] for a color to be included
 */
std::vector<ColorFrequency> color_frequencies(const SegmentedImage& image);

} // namespace glaxnimate::trace
