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
 * \brief Counts pixel values and returns a list of [rgba, count] pairs
 * \param image             The image to analyze
 * \param alpha_threshold   Minimum alpha value [0-255] for a color to be included
 */
std::vector<ColorFrequency> color_frequencies(QImage image, int alpha_threshold = 128);

/**
 * \brief Returns a quantized image with the given colors
 */
QImage quantize(const QImage& source, const std::vector<QRgb>& colors);

} // namespace glaxnimate::utils::quantize
