#pragma once

#include "segmentation.hpp"

namespace glaxnimate::trace {

/**
 * \brief Color frequencies, sorted by count
 */
class ColorFrequencies
{
public:
    using wrapped_type = std::vector<ColorFrequency>;
    using size_type = wrapped_type::size_type;
    using value_type = wrapped_type::value_type;
    using reference = wrapped_type::const_reference;
    using iterator = wrapped_type::const_iterator;

    ColorFrequencies(const SegmentedImage& image);
    ColorFrequencies(const Histogram& histogram);
    std::vector<QRgb> colors(size_type max = 0) const;

    size_type size() const { return freq.size(); }
    iterator begin() const { return freq.begin(); }
    iterator end() const { return freq.end(); }

private:
    wrapped_type freq;
};


/**
 * \brief Returns the \p k colors that appear most frequently in \p image.
 * \pre \p frequencies sorted by count
 */
std::vector<QRgb> k_modes(const ColorFrequencies& frequencies, int k);


enum KMeansMatch
{
    None,
    MostFrequent,
    Closest,
};

/**
 * \brief k-means Algorithm
 * \pre \p frequencies sorted by count
 */
std::vector<QRgb> k_means(const ColorFrequencies& frequencies, int k, int iterations, KMeansMatch match);


/**
 * \brief Octree Algorithm
 * \pre \p frequencies sorted by count
 */
std::vector<QRgb> octree(const ColorFrequencies& frequencies, int k);


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


struct BrushData
{
    std::vector<QRgb> colors = {};
    std::unordered_map<Cluster::id_type, Gradient> gradients = {};
    Gradient gradient(Cluster::id_type id) const
    {
        auto it = gradients.find(id);
        if ( it == gradients.end() )
            return {};
        return it->second;
    }
};

BrushData cluster_merge(SegmentedImage& image, int max_colors, int min_area = 4, int min_color_distance = 16);


} // namespace glaxnimate::trace
