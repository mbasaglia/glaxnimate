#include "quantize.hpp"

#include <unordered_map>
#include <memory>

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

    constexpr Color& operator+=(const Color& oth) noexcept
    {
        weighted_add(oth, 1);
        return *this;
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

/**
 * \note Most of the code here is taken from Inkscape (with several changes)
 * \see https://gitlab.com/inkscape/inkscape/-/blob/master/src/trace/quantize.cpp for the original code
 */
namespace utils::quantize::detail::octree {


inline Color operator>>(Color rgb, int s)
{
  Color res;
  res.r = rgb.r >> s; res.g = rgb.g >> s; res.b = rgb.b >> s;
  return res;
}
inline bool operator==(Color rgb1, Color rgb2)
{
  return (rgb1.r == rgb2.r && rgb1.g == rgb2.g && rgb1.b == rgb2.b);
}

inline int childIndex(Color rgb)
{
    return (((rgb.r)&1)<<2) | (((rgb.g)&1)<<1) | (((rgb.b)&1));
}


struct Node
{
    Node *parent = nullptr;
    std::unique_ptr<Node> children[8];
    // number of children
    int nchild = 0;
    // width level of this node
    int width = 0;
    // rgb's prefix of that node
    Color rgb;
    // number of pixels this node accounts for
    quint32 weight = 0;
    // sum of pixels colors this node accounts for
    Color sum;
    // number of leaves under this node
    int nleaf = 0;
    // minimum impact
    unsigned long mi = 0;


    /**
     * compute the color palette associated to an octree.
     */
    void get_colors(std::vector<QRgb> &colors)
    {
        if (nchild == 0)
        {
            colors.push_back(sum.mean(weight).rgb());
        }
        else
        {
            for (auto & i : children)
                if (i)
                    i->get_colors(colors);
        }
    }

    void update_mi()
    {
        mi = parent ? weight << (2 * parent->width) : 0;
    }
};



/**
 * builds a single <rgb> color leaf
 */
static std::unique_ptr<Node> ocnodeLeaf(Color rgb, quint32 weight)
{
    auto node = std::make_unique<Node>();
    node->width = 0;
    node->rgb = rgb;
    node->sum = Color(rgb.r * weight, rgb.g * weight, rgb.b * weight);
    node->weight = weight;
    node->nleaf = 1;
    node->mi = 0;
    return node;
}


/**
 *  merge nodes <node1> and <node2> at location <ref> with parent <parent>
 */
static std::unique_ptr<Node> octreeMerge(Node *parent, Node *ref, std::unique_ptr<Node> node1, std::unique_ptr<Node> node2)
{
    if (parent && !ref)
        parent->nchild++;

    if ( !node1 )
    {
        node2->parent = parent;
        return node2;
    }

    if ( !node2 )
    {
        node1->parent = parent;
        return node1;
    }

    int dwitdth = node1->width - node2->width;
    if (dwitdth > 0 && node1->rgb == node2->rgb >> dwitdth)
    {
        //place node2 below node1
        node1->parent = parent;

        int i = childIndex(node2->rgb >> (dwitdth - 1));
        node1->sum += node2->sum;
        node1->weight += node2->weight;
        node1->mi = 0;
        if (node1->children[i])
            node1->nleaf -= node1->children[i]->nleaf;

        node1->children[i] = octreeMerge(node1.get(), node1->children[i].get(), std::move(node1->children[i]), std::move(node2));
        node1->nleaf += node1->children[i]->nleaf;
        return node1;
    }
    else if (dwitdth < 0 && node2->rgb == node1->rgb >> (-dwitdth))
    {
        //place node1 below node2
        node2->parent = parent;

        int i = childIndex(node1->rgb >> (-dwitdth - 1));
        node2->sum += node1->sum;
        node2->weight += node1->weight;
        node2->mi = 0;
        if (node2->children[i])
            node2->nleaf -= node2->children[i]->nleaf;
        node2->children[i] = octreeMerge(node2.get(), node2->children[i].get(), std::move(node2->children[i]), std::move(node1));
        node2->nleaf += node2->children[i]->nleaf;
        return node2;
    }
    else
    {
        //nodes have either no intersection or the same root
        auto newnode = std::make_unique<Node>();
        newnode->sum = node1->sum;
        newnode->sum += node2->sum;
        newnode->weight = node1->weight + node2->weight;

        newnode->parent = parent;

        if (dwitdth == 0 && node1->rgb == node2->rgb)
        {
            //merge the nodes in <newnode>
            newnode->width = node1->width; // == node2->width
            newnode->rgb = node1->rgb;     // == node2->rgb
            newnode->nchild = 0;
            newnode->nleaf = 0;
            if (node1->nchild == 0 && node2->nchild == 0)
            {
                newnode->nleaf = 1;
            }
            else
            {
                for (int i = 0; i < 8; i++)
                {
                    if (node1->children[i] || node2->children[i])
                    {
                        newnode->children[i] = octreeMerge(newnode.get(), newnode->children[i].get(), std::move(node1->children[i]), std::move(node2->children[i]));
                        newnode->nleaf += newnode->children[i]->nleaf;
                    }
                }
            }

            return newnode;
        }
        else
        {
            //use <newnode> as a fork node with children <node1> and <node2>
            int newwidth =
              node1->width > node2->width ? node1->width : node2->width;
            Color rgb1 = node1->rgb >> (newwidth - node1->width);
            Color rgb2 = node2->rgb >> (newwidth - node2->width);
            //according to the previous tests <rgb1> != <rgb2> before the loop
            while ( !(rgb1 == rgb2) )
            {
                rgb1 = rgb1 >> 1;
                rgb2 = rgb2 >> 1;
                newwidth++;
            }
            newnode->width = newwidth;
            newnode->rgb = rgb1;  // == rgb2
            newnode->nchild = 2;
            newnode->nleaf = node1->nleaf + node2->nleaf;
            int i1 = childIndex(node1->rgb >> (newwidth - node1->width - 1));
            int i2 = childIndex(node2->rgb >> (newwidth - node2->width - 1));
            node1->parent = newnode.get();
            newnode->children[i1] = std::move(node1);
            node2->parent = newnode.get();
            newnode->children[i2] = std::move(node2);
            return newnode;
        }
    }
}


/**
 * remove leaves whose prune impact value is lower than <lvl>. at most
 * <count> leaves are removed, and <count> is decreased on each removal.
 * all parameters including minimal impact values are regenerated.
 */
static std::unique_ptr<Node> ocnodeStrip(std::unique_ptr<Node> node, int *count, unsigned long lvl)
{
    if ( !count || !node )
        return {};

    if (node->nchild == 0) // leaf node
    {
        if (!node->mi)
            node->update_mi(); //mi generation may be required
        if (node->mi > lvl)
            return node; //leaf is above strip level

        (*count)--;
        return {};
    }
    else
    {
        if (node->mi && node->mi > lvl) //node is above strip level
            return node;
        node->nchild = 0;
        node->nleaf = 0;
        node->mi = 0;
        std::unique_ptr<Node> *lonelychild = nullptr;
        for (auto & child : node->children)
        {
            if ( child )
            {
                child = ocnodeStrip(std::move(child), count, lvl);
                if ( child )
                {
                    lonelychild = &child;
                    node->nchild++;
                    node->nleaf += child->nleaf;
                    if (!node->mi || node->mi > child->mi)
                        node->mi = child->mi;
                }
            }
        }
        // tree adjustments
        if (node->nchild == 0)
        {
            (*count)++;
            node->nleaf = 1;
            node->update_mi();
        }
        else if (node->nchild == 1)
        {
            if ((*lonelychild)->nchild == 0)
            {
                //remove the <lonelychild> leaf under a 1 child node
                node->nchild = 0;
                node->nleaf = 1;
                node->update_mi();
                lonelychild->reset();
            }
            else
            {
                //make a bridge to <lonelychild> over a 1 child node
                (*lonelychild)->parent = node->parent;
                return std::move(*lonelychild);
            }
        }
    }
    return node;
}

/**
 * reduce the leaves of an octree to a given number
 */
static std::unique_ptr<Node> octreePrune(std::unique_ptr<Node> ref, int ncolor)
{
    int n = ref->nleaf - ncolor;
    if ( n <= 0 )
        return {};

    //calling strip with global minimum impact of the tree
    while ( n > 0 && ref )
    {
        auto mi = ref->mi;
        int n1 = n;
        ref = ocnodeStrip(std::move(ref), &n, mi);
        if ( !ref )
            qDebug() << n1 << n;
    }

    return ref;
}


std::unique_ptr<Node> add_pixels(Node* ref, ColorFrequency* data, int data_size)
{
    if ( data_size == 1 )
    {
        return ocnodeLeaf(data->first, data->second);
    }
    else if ( data_size > 1 )
    {
        std::unique_ptr<Node> ref1 = add_pixels(nullptr, data, data_size/2);
        std::unique_ptr<Node> ref2 = add_pixels(nullptr, data + data_size/2, data_size - data_size/2);
        return octreeMerge(nullptr, ref, std::move(ref1), std::move(ref2));
    }

    return {};
}

} // namespace utils::quantize::detail::octree


std::vector<QRgb> utils::quantize::octree(const QImage& image, int k)
{
    using namespace utils::quantize::detail::octree;

    auto freq = color_frequencies(image);

    // Avoid processing if we don't need to
    if ( int(freq.size()) <= k || k <= 1)
    {
        std::sort(freq.begin(), freq.end(), detail::freq_sort_cmp);
        std::vector<QRgb> out;
        out.reserve(k);
        for ( const auto& p : freq )
            out.push_back(p.first);
        return out;
    }

    std::vector<QRgb> colors;
    colors.reserve(k);


    std::unique_ptr<Node> tree = add_pixels(nullptr, freq.data(), freq.size());
    tree = octreePrune(std::move(tree), k);

    tree->get_colors(colors);

    return colors;
}


QImage utils::quantize::quantize(const QImage& source, const std::vector<QRgb>& colors)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QVector<QRgb> vcolors(colors.begin(), colors.end());
#else
    QVector<QRgb> vcolors;
    vcolors.reserve(colors.size()+1);
    for ( auto color : colors )
        vcolors.push_back(color);
#endif
    vcolors.push_back(qRgba(0, 0, 0, 0));
    return source.convertToFormat(QImage::Format_Indexed8, vcolors, Qt::ThresholdDither);
}
