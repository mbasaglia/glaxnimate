#pragma once

#include <QImage>
#include <vector>
#include <unordered_map>

namespace glaxnimate::trace {


using ColorFrequency = std::pair<QRgb, int>;

using Histogram = std::unordered_map<ColorFrequency::first_type, ColorFrequency::second_type>;


struct Cluster
{
    using id_type = int;
    static const constexpr id_type null_id = 0;

    bool is_valid() const;

    // Cluster ID
    id_type id = null_id;
    // Color associated with the cluster
    QRgb color;
    // Number of pixels in the cluster
    int size = 0;
    // Cluster this needs to be merged into
    id_type merge_target = null_id;
    // Clusters that are going to merged into this
    std::vector<id_type> merge_sources = {};
};

struct SegmentedImage
{
    SegmentedImage(int width, int height)
        : width(width), height(height), bitmap(width*height, Cluster::null_id)
    {}

    /**
     * \brief Fixes labels of clusters that need to be merged
     */
    void normalize();

    /**
     * \brief Gets the histogram from the clusters
     */
    Histogram histogram() const;

    /**
     * \brief Returns the id for a new cluster
     */
    Cluster::id_type next_id() const;

    /**
     * \brief Returns a cluster from ID
     */
    Cluster* cluster(Cluster::id_type id);

    /**
     * \brief Returns a cluster from position within the image
     */
    Cluster* cluster(int x, int y);

    /**
     * \brief Returns a cluster ID from position within the image
     */
    Cluster::id_type cluster_id(int x, int y) const;

    /**
     * \brief Merges the two clusters,
     * ensuring merge_target pointers have never a depth larger than 1
     */
    void merge(Cluster* from, Cluster* to);

    /**
     * \brief Merges all clusters with the same color, even if they aren't touching
     */
    void unique_colors(bool flatten_alpha = false);

    /**
     * \brief Adds a new cluster
     */
    Cluster::id_type add_cluster(QRgb color, int size = 1);

    int width;
    int height;
    std::vector<Cluster::id_type> bitmap;
    std::vector<Cluster> clusters;
};

/**
 * \brief Returns a segmented image where each unique pixel is in its own segment
 *
 * Pixels with transparency below \p alpha_threshold will not be in any cluster
 */
SegmentedImage segment(const QImage& image, int alpha_threshold = 1);

} // namespace glaxnimate::trace
