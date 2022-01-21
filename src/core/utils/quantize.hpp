#pragma once


#include <QImage>
#include <vector>

namespace glaxnimate::utils::quantize {


using ColorFrequency = std::pair<QRgb, int>;

using Histogram = std::vector<ColorFrequency>;

/**
 * \brief Returns the \p k colors that appear most frequently in \p image.
 */
std::vector<QRgb> k_modes(const QImage& image, int k);


enum MatchType
{
    Centroid,
    MostFrequent,
    Closest,
};

/**
 * \brief k-means Algorithm
 */
std::vector<QRgb> k_means(const QImage& image, int k, int iterations, MatchType match);


/**
 * \brief Octree Algorithm
 */
std::vector<QRgb> octree(const QImage& image, int k);

/**
 * \brief Counts pixel values and returns a list of [rgba, count] pairs
 * \param image             The image to analyze
 * \param alpha_threshold   Minimum alpha value [0-255] for a color to be included
 */
Histogram color_frequencies(QImage image, int alpha_threshold = 128);

/**
 * \brief Returns a quantized image with the given colors
 */
QImage quantize(const QImage& source, const std::vector<QRgb>& colors);

/**
 * \brief Modified median cut quantization (MMCQ)
 * \see http://www.leptonica.org/color-quantization.html
 */
std::vector<QRgb> modified_median_cut(
    const QImage& image, int k,
    float fract_by_population = 0.85, // try .75 too
    MatchType match = MatchType::Closest,
    int max_iterations = 5000
);

} // namespace glaxnimate::utils::quantize
