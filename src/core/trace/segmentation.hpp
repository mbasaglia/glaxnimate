#pragma once

#include <QImage>
#include <vector>
#include <unordered_map>
#include <unordered_set>


namespace glaxnimate::trace {


using ColorFrequency = std::pair<QRgb, int>;

using Histogram = std::unordered_map<ColorFrequency::first_type, ColorFrequency::second_type>;

/**
 * \brief Distance between two colors
 */
qint32 pixel_distance(QRgb p1, QRgb p2);


/**
 * \brief Returns the index in \p clut that is the closest to \p pixel
 */
qint32 closest_match(QRgb pixel, const std::vector<QRgb> &clut);


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

class SegmentedImage
{
public:
    SegmentedImage(int width, int height)
        : width_(width), height_(height), bitmap_(width*height, Cluster::null_id)
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

    const Cluster* cluster(Cluster::id_type id) const;

    /**
     * \brief Returns a cluster from position within the image
     */
    Cluster* cluster(int x, int y);

    /**
     * \brief Returns a cluster ID from position within the image
     */
    Cluster::id_type cluster_id(int x, int y) const;

    /**
     * \brief Marks the two clusters for merging,
     * ensuring merge_target pointers have never a depth larger than 1
     *
     * Call normalize() to finalize the merge
     */
    void merge(Cluster* from, Cluster* to);

    /**
     * \brief Merges the clusters directly, the \p from cluster will be removed
     * \pre The clusters have no pending merges
     */
    void direct_merge(Cluster::id_type from, Cluster::id_type to);

    /**
     * \brief Merges all clusters with the same color, even if they aren't touching
     */
    void unique_colors(bool flatten_alpha = false);

    /**
     * \brief Adds a new cluster
     */
    Cluster::id_type add_cluster(QRgb color, int size = 1);

//     /**
//      * \brief Marks \p hole as a hole in \p container
//      */
//     void add_hole(Cluster::id_type hole, Cluster::id_type container);

    /**
     * \brief Reduces clusters to match the given colors
     */
    void quantize(const std::vector<QRgb>& colors);

    /**
     * \brief Sorts clusters based on \p func
     * \pre The clusters have no pending merges
     */
    template<class Func>
    void sort_clusters(const Func& func)
    {
        std::sort(clusters_.begin(), clusters_.end(), func);
        fix_cluster_ids();
    }

    /**
     * \brief Fixes cluster IDs if you manually rearrange clusters
     * \pre The clusters have no pending merges
     */
    void fix_cluster_ids();


    /**
     * \brief Dilates a cluster by 1 pixel
     * \param id ID of the cluster to dilate
     * \param protect_size if a pixel is in a cluster with size grater than this, do not dilate
     */
    void dilate(Cluster::id_type id, int protect_size = -1);

    /**
     * \brief Returns the number of pixels in the cluster that mark the boundary of the cluster
     */
    int perimeter(Cluster::id_type id) const;

    /**
     * \brief Converts the segmented image to a QImage
     */
    QImage to_image() const;

    /**
     * \brief Returns a list of IDs of clusters touching pixels of the given cluster
     */
    std::vector<Cluster::id_type> neighbours(Cluster::id_type id) const;

    /**
     * \brief Image width
     */
    int width() const { return width_; }

    /**
     * \brief Image height
     */
    int height() const { return height_; }

    std::vector<Cluster::id_type>& bitmap() { return bitmap_; }
    const std::vector<Cluster::id_type>& bitmap() const { return bitmap_; }
    const std::vector<Cluster>& clusters() const { return clusters_; }
    std::vector<Cluster>& clusters() { return clusters_; }

//     Cluster::id_type hole_parent(Cluster::id_type cluster) const;

private:
    /**
     * \brief Updates cluster IDs
     * \pre new_ids[cluster.id] is the new ID for the given cluster
     */
    void update_cluster_ids(const std::vector<Cluster::id_type>& new_ids, bool update_ids);

    int width_;
    int height_;
    std::vector<Cluster::id_type> bitmap_;
    std::vector<Cluster> clusters_;
//     std::unordered_map<Cluster::id_type, std::unordered_set<Cluster::id_type>> holes_back;
//     std::unordered_map<Cluster::id_type, Cluster::id_type> hole_wrapper;
};

/**
 * \brief Returns a segmented image where each unique pixel is in its own segment
 *
 * Pixels with transparency below \p alpha_threshold will not be in any cluster.
 * If \p diagonal_ajacency is true, diagonal pixels will be considered part of the same cluster.
 */
SegmentedImage segment(const QImage& image, int alpha_threshold = 1, bool diagonal_ajacency = true);

} // namespace glaxnimate::trace
