#pragma once

#include <QImage>
#include <vector>
#include <unordered_map>
#include <unordered_set>


namespace glaxnimate::trace {


using ColorFrequency = std::pair<QRgb, int>;

using Histogram = std::unordered_map<ColorFrequency::first_type, ColorFrequency::second_type>;

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
    using value_type = Cluster;

    SegmentedImage(int width, int height)
        : width_(width), height_(height), bitmap_(width*height, Cluster::null_id)
    {}

    template<class Wrapped>
    class Iterator
    {
    private:
        using c_value_type = std::conditional_t<
            std::is_const_v<std::remove_reference_t<typename Wrapped::reference>>,
            const Cluster,
            Cluster
        >;

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = Cluster;
        using pointer           = c_value_type*;
        using reference         = c_value_type&;

        reference operator*() const { return iter->second; }
        pointer operator->() const { return &iter->second; }

        Iterator& operator++() { ++iter; return *this; }
        Iterator operator++(int) { Iterator tmp = *this; ++iter; return tmp; }

        bool operator== (const Iterator& b) { return iter == b.iter; }
        bool operator!= (const Iterator& b) { return iter != b.iter; }

    private:
        Iterator(Wrapped iter) : iter(std::move(iter)) {}
        Wrapped iter;
        friend SegmentedImage;
    };

    using iterator = Iterator<std::unordered_map<Cluster::id_type, Cluster>::iterator>;
    using const_iterator = Iterator<std::unordered_map<Cluster::id_type, Cluster>::const_iterator>;

    /**
     * \brief Fixes labels of clusters that need to be merged
     */
    void normalize();

    /**
     * \brief Gets the histogram from the clusters
     */
    Histogram histogram() const;

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
    Cluster* add_cluster(QRgb color, int size = 1);

//     /**
//      * \brief Marks \p hole as a hole in \p container
//      */
//     void add_hole(Cluster::id_type hole, Cluster::id_type container);

    /**
     * \brief Reduces clusters to match the given colors
     */
    void quantize(const std::vector<QRgb>& colors);

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

    /**
     * \brief Collapses all clusters into one
     */
    template<class Callback>
    Cluster* mono(const Callback& callback)
    {
        auto final_cluster = add_cluster(0x0, 0);
        for ( auto it = clusters_.begin(); it != clusters_.end(); )
        {
            if ( callback(it->second) )
            {
                merge(&it->second, final_cluster);
                ++it;
            }
            else
            {
                it = clusters_.erase(it);
            }
        }

        normalize();
        return final_cluster;
    }

    template<class Callback>
    void erase_if(const Callback& callback)
    {
        std::unordered_set<Cluster::id_type> deleted;

        for ( auto it = clusters_.begin(); it != clusters_.end(); )
        {
            if ( callback(it->second) )
            {
                deleted.insert(it->first);
                it = clusters_.erase(it);
            }
            else
            {
                ++it;
            }
        }

        for ( auto& pix : bitmap_ )
            if ( deleted.count(pix) )
                pix = Cluster::null_id;
    }

    std::vector<Cluster::id_type>& bitmap() { return bitmap_; }
    const std::vector<Cluster::id_type>& bitmap() const { return bitmap_; }

    const_iterator begin() const { return const_iterator{clusters_.begin()}; }
    const_iterator end() const { return const_iterator{clusters_.end()}; }
    const_iterator cbegin() const { return const_iterator{clusters_.begin()}; }
    const_iterator cend() const { return const_iterator{clusters_.end()}; }
    iterator begin() { return iterator{clusters_.begin()}; }
    iterator end() { return iterator{clusters_.end()}; }

    /**
     * \brief Number of clusters
     */
    int size() const { return clusters_.size(); }
//     Cluster::id_type hole_parent(Cluster::id_type cluster) const;

private:
    int width_;
    int height_;
    std::vector<Cluster::id_type> bitmap_;
    std::unordered_map<Cluster::id_type, Cluster> clusters_;
    Cluster::id_type next_id = 1;
};

/**
 * \brief Returns a segmented image where each unique pixel is in its own segment
 *
 * Pixels with transparency below \p alpha_threshold will not be in any cluster.
 * If \p diagonal_ajacency is true, diagonal pixels will be considered part of the same cluster.
 */
SegmentedImage segment(const QImage& image, bool diagonal_ajacency = true);

} // namespace glaxnimate::trace
