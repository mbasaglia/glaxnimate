#pragma once

#include <QImage>
#include <QDebug>

#include <unordered_set>
#include <memory>

#include "trace/color.hpp"


namespace glaxnimate::trace {


struct Cluster
{
    using id_type = int;
    static const constexpr id_type null_id = 0;

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
    // Start/end indexes in the bitmap
    std::size_t index_start = 0;
    std::size_t index_end = 0;

    QString to_string() const;
};

class SegmentedImage
{
private:
    using SparseVector = std::vector<std::unique_ptr<Cluster>>;

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

        reference operator*() const { return *iter->get(); }
        pointer operator->() const { return iter->get(); }

        Iterator& operator++()
        {
            do
                ++iter;
            while ( iter != end && !*iter );
            return *this;
        }
        Iterator operator++(int) { Iterator tmp = *this; ++*this; return tmp; }

        bool operator== (const Iterator& b) { return iter == b.iter; }
        bool operator!= (const Iterator& b) { return iter != b.iter; }

    private:
        Iterator(Wrapped iter, Wrapped end)
        : iter(std::move(iter)), end(std::move(end))
        {
            if ( iter != end && !*iter )
                ++*this;
        }

        Wrapped iter;
        Wrapped end;
        friend SegmentedImage;
    };

    class PixelRange
    {
    public:
        using iterator = std::vector<Cluster::id_type>::iterator;
        using value_type = std::vector<Cluster::id_type>::value_type;

        const iterator& begin() const { return begin_; }
        const iterator& end() const { return end_; }

        iterator begin_;
        iterator end_;
        friend SegmentedImage;
    };

public:
    using value_type = Cluster;

    SegmentedImage(int width, int height)
        : width_(width), height_(height), bitmap_(width*height, Cluster::null_id)
    {
        clusters_.emplace_back();
    }

    SegmentedImage(const SegmentedImage& o)
        : width_(o.width_),
          height_(o.height_),
          bitmap_(o.bitmap_),
          next_id(o.next_id),
          size_(o.size_)
    {
        clusters_.reserve(o.clusters_.size());
        for ( const auto& c : o.clusters_ )
            if ( c )
                clusters_.emplace_back(std::make_unique<Cluster>(*c));
            else
                clusters_.emplace_back();

    }
    SegmentedImage(SegmentedImage&& o) = default;
    SegmentedImage& operator=(const SegmentedImage& o)
    {
        auto copy = o;
        *this = std::move(copy);
        return *this;
    }
    SegmentedImage& operator=(SegmentedImage&& o) = default;

    using iterator = Iterator<SparseVector::iterator>;
    using const_iterator = Iterator<SparseVector::const_iterator>;


    const iterator erase(iterator iter)
    {
        do_erase(*iter.iter);
        return ++iter;
    }

    /**
     * \brief Fixes labels of clusters that need to be merged
     */
    void normalize();

    /**
     * \brief Gets the histogram from the clusters
     */
    Histogram histogram(bool flatten_lapha = false) const;

    /**
     * \brief Returns a cluster from ID
     */
    Cluster* cluster(Cluster::id_type id)
    {
        return clusters_[id].get();
    }

    const Cluster* cluster(Cluster::id_type id) const
    {
        return clusters_[id].get();
    }

    /**
     * \brief Returns a cluster from position within the image
     */
    Cluster* cluster(int x, int y)
    {
        return cluster(cluster_id(x, y));
    }

    /**
     * \brief Returns a cluster ID from position within the image
     */
    Cluster::id_type cluster_id(int x, int y) const
    {
        if ( x < 0 || y < 0 || x >= width_ || y >= height_ )
            return Cluster::null_id;
        return bitmap_[x + y * width_];
    }

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
     * \param clhster Cluster to dilate
     * \param protect_size if a pixel is in a cluster with size grater than this, do not dilate
     */
    void dilate(Cluster* cluster, int protect_size = -1);

    /**
     * \brief Returns the number of pixels in the cluster that mark the boundary of the cluster
     */
    int perimeter(Cluster* cluster) const;

    /**
     * \brief Converts the segmented image to a QImage
     */
    QImage to_image() const;

    /**
     * \brief Returns a list of IDs of clusters touching pixels of the given cluster
     */
    std::vector<Cluster::id_type> neighbours(Cluster* cluster) const;

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
        for ( auto& it : clusters_ )
        {
            if ( it )
            {
                if ( callback(*it) )
                    merge(it.get(), final_cluster);
                else
                    it.release();
            }
        }

        normalize();
        return final_cluster;
    }

    template<class Callback>
    void erase_if(const Callback& callback)
    {
        std::unordered_set<Cluster::id_type> deleted;

        for ( auto& it : clusters_ )
        {
            if ( it && callback(*it) )
            {
                deleted.insert(it->id);
                do_erase(it);
            }
        }

        for ( auto& pix : bitmap_ )
            if ( deleted.count(pix) )
                pix = Cluster::null_id;
    }

    std::vector<Cluster::id_type>& bitmap() { return bitmap_; }
    const std::vector<Cluster::id_type>& bitmap() const { return bitmap_; }

    const_iterator begin() const { return const_iterator{clusters_.begin(), clusters_.end()}; }
    const_iterator end() const { return const_iterator{clusters_.end(), clusters_.end()}; }
    const_iterator cbegin() const { return const_iterator{clusters_.begin(), clusters_.end()}; }
    const_iterator cend() const { return const_iterator{clusters_.end(), clusters_.end()}; }
    iterator begin() { return iterator{clusters_.begin(), clusters_.end()}; }
    iterator end() { return iterator{clusters_.end(), clusters_.end()}; }

    /**
     * \brief Initializes clusters based on pixel data
     * \pre pixels points to at least width()*height() pixels, arranged by rows of width()
     */
    void segment(const quint32* pixels, bool diagonal_ajacency);

    /**
     * \brief Number of clusters
     */
    int size() const
    {
        return size_;
    }

//     Cluster::id_type hole_parent(Cluster::id_type cluster) const;

    /**
     * \brief Returns a range of pixels in bitmap() that cointain the cluster
     *
     * All pixels in \p cluster are in the range but some pixels in the range might not be in \p cluster
     */
    PixelRange pixel_range(Cluster* cluster)
    {
        return PixelRange{bitmap_.begin() + cluster->index_start, bitmap_.begin() + cluster->index_end + 1};
    }

private:
    Cluster* cluster_by_offset(std::size_t offset)
    {
        return clusters_[bitmap_[offset]].get();
    }

    void do_erase(SparseVector::value_type& ptr)
    {
        ptr.reset();
        size_--;
    }

    void merge_attributes(Cluster* from, Cluster* to);

    int width_;
    int height_;
    std::vector<Cluster::id_type> bitmap_;
    SparseVector clusters_;
    Cluster::id_type next_id = 1;
    int size_ = 0;
};

/**
 * \brief Returns a segmented image where each unique pixel is in its own segment
 *
 * Pixels with transparency below \p alpha_threshold will not be in any cluster.
 * If \p diagonal_ajacency is true, diagonal pixels will be considered part of the same cluster.
 */
SegmentedImage segment(const QImage& image, bool diagonal_ajacency = true);

} // namespace glaxnimate::trace

QDebug operator<<(QDebug db, const glaxnimate::trace::Cluster& cluster);
