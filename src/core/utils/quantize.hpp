/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once


#include <QImage>
#include <vector>

namespace glaxnimate::utils::quantize {


using ColorFrequency = std::pair<QRgb, int>;

/**
 * \brief Returns the \p k colors that appear most frequently in \p image.
 */
std::vector<QRgb> k_modes(const QImage& image, int k);


enum KMeansMatch
{
    None,
    MostFrequent,
    Closest,
};

/**
 * \brief k-means Algorithm
 */
std::vector<QRgb> k_means(const QImage& image, int k, int iterations, KMeansMatch match);


/**
 * \brief Octree Algorithm
 */
std::vector<QRgb> octree(const QImage& image, int k);


/**
 * \brief Edge cutoff
 *
 * Selects the most frequent color, then excludes areas within 1 pixel of that color
 * Then it repeats until all the colors have been found
 *
 * \param image Image to get the colors for
 * \param max_colors Maximum number of colors
 * \param min_frequency A color must have at least min_frequency * image.width * image.height pixels to be selected
 */
std::vector<QRgb> edge_exclusion_modes(const QImage& image, int max_colors, qreal min_frequency = 0.0005);

/**
 * \brief Counts pixel values and returns a list of [rgba, count] pairs
 * \param image             The image to analyze
 * \param alpha_threshold   Minimum alpha value [0-255] for a color to be included
 */
std::vector<ColorFrequency> color_frequencies(const QImage& image, int alpha_threshold = 128);

/**
 * \brief Returns a quantized image with the given colors
 */
QImage quantize(const QImage& source, const std::vector<QRgb>& colors);

} // namespace glaxnimate::utils::quantize
