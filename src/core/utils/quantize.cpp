#include "quantize.hpp"


std::vector<utils::quantize::ColorFrequency> utils::quantize::color_frequencies(QImage image, int alpha_threshold)
{
    if ( image.format() != QImage::Format_RGBA8888 )
        image = image.convertToFormat(QImage::Format_RGBA8888);

    std::unordered_map<QRgb, int> count;
    const uchar* data = image.constBits();

    int n_pixels = image.width() * image.height();
    for ( int i = 0; i < n_pixels; i++ )
        if ( data[i*4+3] >= alpha_threshold )
            ++count[qRgb(data[i*4], data[i*4+1], data[i*4+2])];

    return std::vector<ColorFrequency>(count.begin(), count.end());
}

namespace utils::quantize::detail {

static bool freq_sort_cmp(const ColorFrequency& a, const ColorFrequency& b) noexcept
{
    return a.second > b.second;
}


using Distance = quint32;

struct Color
{
    qint32 r;
    qint32 g;
    qint32 b;

    constexpr Color(QRgb rgb) noexcept
    : r(qRed(rgb)), g(qGreen(rgb)), b(qBlue(rgb))
    {}

    constexpr Color() noexcept
    : r(0), g(0), b(0)
    {}

    constexpr Color(qint32 r, qint32 g, qint32 b) noexcept
    : r(r), g(g), b(b)
    {}

    constexpr Distance distance(const Color& oth) const noexcept
    {
        quint32 dr = r - oth.r;
        quint32 dg = g - oth.g;
        quint32 db = b - oth.b;

        return dr * dr + dg * dg + db * db;
    }

    constexpr QRgb rgb() const noexcept
    {
        return qRgb(r, g, b);
    }

    constexpr void weighted_add(const Color& oth, int weight) noexcept
    {
        r += oth.r * weight;
        g += oth.g * weight;
        b += oth.b * weight;
    }

    Color mean(qreal total_weight) const noexcept
    {
        return {
            qRound(r / total_weight),
            qRound(g / total_weight),
            qRound(b / total_weight)
        };
    }
};

} // utils::quantize::detail

std::vector<QRgb> utils::quantize::k_modes(const QImage& image, int k)
{
    auto sortme = color_frequencies(image);
    std::sort(sortme.begin(), sortme.end(), detail::freq_sort_cmp);

    std::vector<QRgb> out;
    if ( int(sortme.size()) < k )
        k = sortme.size();
    out.reserve(k);
    for ( int i = 0; i < qMin<int>(k, sortme.size()); i++ )
        out.push_back(sortme[i].first);

    return out;
}



namespace utils::quantize::detail::k_means {

struct Point
{
    Color color;

    quint32 weight;

    Distance min_distance = std::numeric_limits<Distance>::max();

    int cluster = -1;


    constexpr Point(const ColorFrequency& p) noexcept
        : color(p.first), weight(p.second)
    {}
};


struct Cluster
{
    Color centroid;
    quint32 total_weight = 0;
    Color sum = {};

    constexpr Cluster(const Color& color) noexcept
        : centroid(color)
    {}

    bool update()
    {
        if ( total_weight == 0 )
            return false;

        auto old = centroid;

        centroid.r = qRound(double(sum.r) / total_weight);
        centroid.g = qRound(double(sum.g) / total_weight);
        centroid.b = qRound(double(sum.b) / total_weight);

        total_weight = 0;
        sum = {};
        return old.rgb() != centroid.rgb();
    }
};

} // utils::quantize::detail

std::vector<QRgb> utils::quantize::k_means(const QImage& image, int k, int iterations, KMeansMatch match)
{
    auto freq = color_frequencies(image);

    // Avoid processing if we don't need to
    if ( int(freq.size()) <= k )
    {
        std::sort(freq.begin(), freq.end(), detail::freq_sort_cmp);
        std::vector<QRgb> out;
        out.reserve(k);
        for ( const auto& p : freq )
            out.push_back(p.first);
        return out;
    }

    // Initialize points
    std::vector<detail::k_means::Point> points(freq.begin(), freq.end());
    freq.clear();

    // Keep track of the clusters we already used
    std::vector<detail::k_means::Point> cluster_init;
    cluster_init.reserve(k);

    std::vector<detail::k_means::Cluster> clusters;
    clusters.reserve(k);

    // Get the most common color as initial cluster centroid
    quint32 max_freq = 0;
    std::vector<detail::k_means::Point>::iterator best_iter;
    for ( auto it = points.begin(); it != points.end(); ++it )
    {
        if ( it->weight > max_freq )
        {
            max_freq = it->weight;
            best_iter = it;
        }
    }
    cluster_init.push_back(*best_iter);
    clusters.emplace_back(best_iter->color);
    // remove centroid from points
    std::swap(*best_iter, points.back());
    points.pop_back();

    // k-means++-like processing from now on (but deterministic)
    // ie: always select the centroid the farthest away from the other centroids
    while ( int(clusters.size()) < k )
    {
        detail::Distance max_dist = 0;

        for ( auto it = points.begin(); it != points.end(); ++it )
        {
            detail::Distance p_max = 0;
            for ( const auto& cluster : clusters )
            {
                auto dist = it->color.distance(cluster.centroid);
                if ( dist < p_max )
                    p_max = dist;
            }

            if ( p_max > max_dist )
            {
                max_dist = p_max;
                best_iter = it;
            }
        }

        cluster_init.push_back(*best_iter);
        clusters.emplace_back(best_iter->color);
        // remove centroid from points
        std::swap(*best_iter, points.back());
        points.pop_back();
    }

    // add back the removed centroids
    points.insert(points.end(), cluster_init.begin(), cluster_init.end());
    cluster_init.clear();


    // K-medoids
    bool loop = true;
    for ( int epoch = 0; epoch < iterations && loop; epoch++ )
    {
        // Assign points to clusters
        for ( int i = 0; i < k; i++ )
        {
            const auto& cluster = clusters[i];
            for ( auto& p : points )
            {
                auto dist = p.color.distance(cluster.centroid);
                if (dist < p.min_distance )
                {
                    p.min_distance = dist;
                    p.cluster = i;
                }
            }
        }

        // Move centroids
        for ( auto& p : points )
        {
            clusters[p.cluster].total_weight += p.weight;
            clusters[p.cluster].sum.weighted_add(p.color, p.weight);
            p.min_distance = std::numeric_limits<detail::Distance>::max();
        }

        // Quit if nothing has changed
        loop = false;
        for ( auto& cluster : clusters )
            loop = cluster.update() || loop;
    }

    // Post-process to find the closest color, reusing total_weight/sum for this
    if ( match )
    {
        for ( auto& cluster : clusters )
        {
            cluster.total_weight = 0;
            cluster.sum = {};
        }


        for ( auto& p : points )
        {
            auto& cluster = clusters[p.cluster];
            auto score = match == MostFrequent ? p.weight :
                std::numeric_limits<quint32>::max() - p.color.distance(cluster.centroid);

            if ( score > cluster.total_weight )
            {
                cluster.total_weight = score;
                cluster.sum = p.color;
            }
        }

        for ( auto& cluster : clusters )
        {
            cluster.centroid = cluster.sum;
        }
    }

    std::vector<QRgb> result;
    result.reserve(k);
    for ( auto& cluster : clusters )
        result.push_back(cluster.centroid.rgb());

    return result;
}

#include <memory>
#include <unordered_set>
#include <QDebug>
namespace utils::quantize::detail::octree {


constexpr const int max_depth = 8;

/**
* \return The \p nth bit of \p value.
*/
int bit(int nth, int value) noexcept
{
    return (value >> nth) & 1;
}

struct Node;

struct Data
{
    std::vector<Node*> nodes;
    int leaves = 0;
};

struct Node
{
    struct MinChild
    {
        int index = -1;
        qint32 weight = std::numeric_limits<qint32>::max();
    };

    Node* parent = nullptr;
    std::vector<std::unique_ptr<Node>> children;
    qint32 weight = 0;
    std::vector<ColorFrequency> colors;
    int level = -1;
    MinChild min;
    MinChild min2;
    int index;
    Data* data;

    Node(Node* parent, int depth, Data* data)
        : parent(parent), level(depth), index(data->nodes.size()), data(data)
    {
        data->nodes.push_back(this);
        data->leaves++;
    }

    ~Node()
    {
        data->nodes[index] = nullptr;
    }

    /**
     * \brief Whether the node is a leaf
     */
    bool leaf() const
    {
        return children.empty();
    }

    /**
     * \brief Returns the child at \p index.
     * If that child is null, it gets constructed
     * \pre !leaf()
     */
    Node* child(int index)
    {
        if ( !children[index] )
            children[index] = std::make_unique<Node>(this, level+1, data);
        return children[index].get();
    }

    /**
     * \brief Inserts a color into the subtree
     */
    void insert(const ColorFrequency& freq)
    {
        weight += freq.second;

        if ( leaf() )
        {
            if ( colors.size() == 0 || level == max_depth )
            {
                colors.push_back(freq);
                return;
            }

            // We no longer want to be a leaf
            children.resize(8);
            data->leaves--;
            insert_into_child(colors[0]);
            colors.clear();
        }

        insert_into_child(freq);
    }

    /**
     * \brief Average weighted color
     * \pre leaf()
     */
    Color mean_color() const
    {
        Color sum;
        for ( const auto& c : colors )
            sum.weighted_add(c.first, c.second);

        return sum.mean(weight);
    }

    /**
     * \brief Best color based on the match mode
     * \pre leaf()
     */
    QRgb best_color(KMeansMatch match) const
    {
        switch ( match )
        {
            case None:
                return mean_color().rgb();
            case MostFrequent:
            {
                ColorFrequency best{0, 0};
                for ( const auto& c : colors )
                {
                    if ( c.second > best.second )
                        best = c;
                }

                return best.first;
            }
            case Closest:
            {
                Color best;
                Distance closest = std::numeric_limits<Distance>::max();
                for ( const auto& c : colors )
                {
                    auto dist = best.distance(c.first);
                    if ( dist < closest )
                    {
                        closest = dist;
                        best = c.first;
                    }
                }
                return best.rgb();
            }
        }

        return {};
    }

    /**
     * \brief Populates \p out with best_color for every leaf
     */
    void get_colors(std::vector<QRgb>& out, KMeansMatch match) const
    {
        if ( leaf() )
        {
            out.push_back(best_color(match));
        }
        else
        {
            for ( const auto& ch : children )
                ch->get_colors(out, match);
        }
    }

    /**
     * \brief Debug print
     */
    void debug(QString indent)
    {
        auto col = mean_color();
        qDebug().noquote() << indent << level << index << weight << leaf() << col.r << col.g << col.b;

        indent += "    ";
        for ( const auto& ch : children )
        {
            if ( ch )
                ch->debug(indent);
        }
    }

    /**
     * \brief Reduces leaves by one, merging the two smallest nodes
     * \pre pre_process() called && leaves > 1
     */
    void merge()
    {
        do_merge();
    }

    /**
     * \brief Perform some pre-processing before reducinbg the tree
     *
     * Removes null children and updates min references, ensure there is no node with an only child
     */
    void pre_process()
    {
        children.erase(std::remove(children.begin(), children.end(), nullptr), children.end());

        for ( const auto& ch : children )
            ch->pre_process();

        if ( children.size() == 1 )
        {
            if ( children[0]->leaf() )
            {
                merge_data_from(children[0].get(), false);
                children.clear();
            }
            else
            {
                min = children[0]->min;
                min2 = children[0]->min2;
                children = std::move(children[0]->children);
            }
        }
        else
        {
            update_min();
        }
    }

    /**
     * \brief Returns the sum of the two children with the minimum weight
     * \pre !leaf()
     */
    qint32 min_sum() const noexcept
    {
        return min.weight + min2.weight;
    }


private:
    /**
     * \brief Inserts \p freq in a child
     * \pre level < max_depth
     */
    void insert_into_child(const ColorFrequency& freq)
    {
        int index = child_index(freq.first);
        child(index)->insert(freq);
    }

    /**
     * \brief Propagate a change in the number of leaves
     */
    void update_leaves(int delta)
    {
        data->leaves += delta;
    }

    /**
     * \brief Updates minimum and second-minimum children
     */
    void update_min()
    {
        min = min2 = {};

        for ( int index = 0; index < int(children.size()); index++ )
            update_min(index);
    }

    /**
     * \brief Updates minimum and second-minimum children for the given child index
     */
    void update_min(int index)
    {
        auto weight = children[index]->weight;

        if ( min.index == -1 )
        {
            min = {index, weight};
        }
        else if ( weight < min.weight )
        {
            min2 = min;
            min = {index, weight};
        }
        else if ( min2.index == -1 )
        {
            min2 = {index, weight};
        }
    }

    /**
     * \brief Child index of \p color.
     */
    int child_index(const Color& color) noexcept
    {
        return bit(max_depth - level, color.r) * 4 +
            bit(max_depth - level, color.g) * 2 +
            bit(max_depth - level, color.b);
    }

    void do_merge()
    {
        // no other nodes => merge all in the current node
        if ( children.size() == 2 )
        {
            make_leaf();
            return;
        }


        Node* ch1 = children[min.index].get();
        Node* ch2 = children[min2.index].get();

        // Merge a leaf into another
        if ( ch1->leaf() && ch2->leaf() )
        {
            ch2->merge_data_from(ch1, true);
            children.erase(children.begin() + min.index);
            update_leaves(-1);
            update_min();
            return;

        }

        // Merge ch1 with the min child of ch2
        if ( ch1->leaf() )
        {
            ch2->merge_cousin(ch1);
            children.erase(children.begin() + min.index);
            update_leaves(-1);
            update_min();
            return;
        }

        if ( ch2->leaf() )
        {
            ch1->merge_cousin(ch2);
            children.erase(children.begin() + min2.index);
            update_leaves(-1);
            update_min();
            return;
        }

        ch2->make_leaf();
        ch1->merge_all_into(ch2, true);
        children.erase(children.begin() + min.index);
        update_leaves(-1);
        update_min();
        return;
    }

    /**
     * \brief Merge a node that has a common ancestor with this
     */
    void merge_cousin(Node* cousin)
    {
        Node* ch = children[min.index].get();
        if ( ch->leaf() )
            cousin->merge_all_into(ch, true);
        else
            ch->merge_cousin(cousin);

        update_min();

    }

    /**
     * \brief Copies some data from \p other.
     */
    void merge_data_from(Node* other, bool merge_weight)
    {
        if ( merge_weight )
            weight += other->weight;
        colors.insert(colors.end(), other->colors.begin(), other->colors.end());
        if ( leaf() && colors.empty() )
            qDebug() << index;
    }

    /**
     * \brief Turns a node with 2 children into a leaf \pre !leaf()
     */
    void make_leaf()
    {
        merge_all_into(this, false);
        update_leaves(1-sub_leaves());
        if ( colors.empty() ) {
            qDebug() << index;
            merge_all_into(this, false);
        }
        children.clear();

    }

    void merge_all_into(Node* other, bool merge_weight)
    {
        if ( leaf() )
            other->merge_data_from(this, merge_weight);
        for ( const auto& ch : children )
            ch->merge_all_into(other, merge_weight);
    }

    int sub_leaves()
    {
        int total = 0;
        for ( const auto& ch : children )
        {
            if ( ch->leaf() )
                total += 1;
            else
                total += ch->sub_leaves();
        }
        return total;
    }
};



} // namespace utils::quantize::detail::octree

std::vector<QRgb> utils::quantize::octree(const QImage& image, int k, KMeansMatch match)
{
    auto freq = color_frequencies(image);
    std::vector<QRgb> out;
    out.reserve(k);

    // Avoid processing if we don't need to
    if ( int(freq.size()) <= k || k <= 1 )
    {
        std::sort(freq.begin(), freq.end(), detail::freq_sort_cmp);
        for ( const auto& p : freq )
            out.push_back(p.first);
        return out;
    }

    detail::octree::Data octree_data;
    octree_data.nodes.reserve(freq.size());
    detail::octree::Node root{nullptr, 1, &octree_data};

    for ( const auto& cf : freq )
        root.insert(cf);
    freq.clear();


    root.pre_process();

    while ( octree_data.leaves > k )
    {
        qint32 best = std::numeric_limits<qint32>::max();
        detail::octree::Node* merger = nullptr;
        for ( auto node : octree_data.nodes )
        {
            if ( node && !node->leaf() )
            {
                auto min = node->min_sum();
                if ( min < best )
                {
                    best = min;
                    merger = node;
                    if ( min == 2 )
                        break;
                }
            }
        }

        merger->merge();
    }

    root.get_colors(out, match);
    return out;
}
