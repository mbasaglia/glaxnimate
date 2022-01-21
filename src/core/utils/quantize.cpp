#include "quantize.hpp"

#include <unordered_map>
#include <memory>
#include <queue>

#include <QDebug>

using namespace glaxnimate;

glaxnimate::utils::quantize::Histogram utils::quantize::color_frequencies(QImage image, int alpha_threshold)
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

namespace glaxnimate::utils::quantize::detail {

static bool freq_sort_cmp(const ColorFrequency& a, const ColorFrequency& b) noexcept
{
    return a.second > b.second;
}

std::vector<QRgb> color_frequencies_to_palette(Histogram& freq, int max)
{
    std::sort(freq.begin(), freq.end(), detail::freq_sort_cmp);

    int count = qMin<int>(max, freq.size());
    std::vector<QRgb> out;
    out.reserve(count);

    for ( int i = 0; i < count; i++ )
        out.push_back(freq[i].first);

    return out;
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
    return detail::color_frequencies_to_palette(sortme, k);
}



namespace glaxnimate::utils::quantize::detail::k_means {

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

std::vector<QRgb> utils::quantize::k_means(const QImage& image, int k, int iterations, MatchType match)
{
    auto freq = color_frequencies(image);

    // Avoid processing if we don't need to
    if ( int(freq.size()) <= k )
        return detail::color_frequencies_to_palette(freq, k);

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
namespace glaxnimate::utils::quantize::detail::octree {


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
        ref = ocnodeStrip(std::move(ref), &n, mi);
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

} // namespace glaxnimate::utils::quantize::detail::octree


std::vector<QRgb> utils::quantize::octree(const QImage& image, int k)
{
    using namespace glaxnimate::utils::quantize::detail::octree;

    auto freq = color_frequencies(image);

    // Avoid processing if we don't need to
    if ( int(freq.size()) <= k || k <= 1)
    {
        return detail::color_frequencies_to_palette(freq, k);
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

namespace glaxnimate::utils::quantize::detail::mmcq {

struct VBox
{
    Color min;
    Color max;
    Histogram* histogram;
    qint32 volume;
    qint32 count;

    VBox(const Color& min, const Color& max, Histogram* histogram)
        : min(min), max(max), histogram(histogram)
    {
        refresh_count();
        refresh_volume();
    }

    void refresh_volume()
    {
        volume = (max.r - min.r + 1) * (max.g - min.g + 1) * (max.b - min.b + 1);
    }

    void refresh_count()
    {
        count = 0;
        for ( const auto& freq : *histogram )
        {
            int r = qRed(freq.first);
            int g = qRed(freq.first);
            int b = qRed(freq.first);
            if ( r >= min.r && r <= max.r && g >= min.g && g <= max.g && b >= min.b && b <= min.b )
                count += freq.second;
        }
    }

    bool operator<(const VBox& other) const
    {
        return volume * count < other.volume * other.count;
    }

    static VBox from_histogram(Histogram* histogram)
    {
        Color min(255, 255, 255);
        Color max(0, 0, 0);
        for ( const auto& f : *histogram )
        {
            auto r = qRed(f.first);
            auto g = qGreen(f.first);
            auto b = qBlue(f.first);

            if ( r > max.r ) max.r = r;
            else if ( r < min.r ) min.r = r;

            if ( g > max.g ) max.g = g;
            else if ( g < min.g ) min.g = g;

            if ( b > max.b ) max.b = b;
            else if ( b < min.b ) min.b = b;
        }

        return VBox{min, max, histogram};
    }

    QRgb average(MatchType match) const
    {
        qint32 total = 0;
        qint32 rsum = 0;
        qint32 gsum = 0;
        qint32 bsum = 0;
        qint32 most = 0;
        Color selected;

        for ( const auto& freq : *histogram )
        {
            int r = qRed(freq.first);
            int g = qGreen(freq.first);
            int b = qBlue(freq.first);
            if ( r >= min.r && r <= max.r && g >= min.g && g <= max.g && b >= min.b && b <= min.b )
            {
                total += freq.second;
                rsum += freq.second * r;
                gsum += freq.second * g;
                bsum += freq.second * b;
                if ( freq.second > most )
                {
                    selected = freq.first;
                    most = freq.second;
                }
            }
        }

        if ( match == MostFrequent )
            return selected.rgb();

        Color median;
        if ( total == 0 )
        {
            median = Color(
                (min.r + max.r + 1) / 2,
                (min.g + max.g + 1) / 2,
                (min.b + max.b + 1) / 2
            );
        } else {
            median = Color(
                rsum / total,
                gsum / total,
                bsum / total
            );
        }

        if ( match == Centroid )
            return median.rgb();


        auto closest = std::numeric_limits<quint32>::max();
        for ( const auto& freq : *histogram )
        {
            Color col(freq.first);
            auto dist = col.distance(median);
            if ( dist < closest )
            {
                closest = dist;
                selected = col;
            }
        }

        return selected.rgb();
    }
};

struct VBoxCompareCount
{
    bool operator()(const VBox& a, const VBox& b) const
    {
        return a.count < b.count;
    }
};

std::vector<VBox> median_cut_apply(Histogram* histogram, const VBox& vbox)
{
    if ( !vbox.count )
        return {};

    if ( vbox.count == 1 )
        return {vbox};

    Color width(
        vbox.max.r - vbox.min.r + 1,
        vbox.max.g - vbox.min.g + 1,
        vbox.max.b - vbox.min.b + 1
    );

    auto max_width = std::max(width.r, std::max(width.g, width.b));
    std::vector<qint32> partial_sum;
    partial_sum.resize(max_width);
    qint32 Color::* comp;

    qint32 total = 0;

    if ( max_width == width.r )
        comp = &Color::r;
    else if ( max_width == width.g )
        comp = &Color::g;
    else
        comp = &Color::b;

    for ( const auto& freq : *histogram )
    {
        Color col(freq.first);
        if ( col.r >= vbox.min.r && col.r <= vbox.max.r &&
                col.g >= vbox.min.g && col.g <= vbox.max.g &&
                col.b >= vbox.min.b && col.b <= vbox.min.b )
        {
            total += freq.second;
            partial_sum[col.*comp] += freq.second;
        }
    }

    for ( std::size_t i = 1; i < partial_sum.size(); i++ )
        partial_sum[i] += partial_sum[i-1];

    for ( int i = vbox.min.*comp; i <= vbox.max.*comp; i++)
    {
        if ( partial_sum[i] > total / 2)
        {
            VBox vbox1 = vbox;
            VBox vbox2 = vbox;
            int left = i - vbox.min.*comp;
            int right = vbox.max.*comp - i;
            int split;

            if (left <= right)
                split = std::min(vbox.max.*comp - 1, i + right / 2);
            else
                split = std::min(vbox.min.*comp, i - 1 - left / 2);

            vbox1.max.*comp = split;
            vbox2.min.*comp = split + 1;

            vbox1.count = partial_sum[split];
            vbox1.refresh_volume();

            vbox2.count = total - partial_sum[split];
            vbox2.refresh_volume();

            return {vbox1, vbox2};
        }
    }

    return {};
}


template<class Comp>
void iterate(std::priority_queue<VBox, std::vector<VBox>, Comp>& queue, Histogram* histogram, qint32 target, int max_iterations)
{
    qint32 n_colors = 1;

    for ( int i = 0; i < max_iterations; i++ )
    {
        auto vbox = queue.top();
        if ( !vbox.count )
            continue;

        queue.pop();

        auto boxes = median_cut_apply(histogram, vbox);
        if ( boxes.empty() )
            continue;

        queue.push(boxes[0]);

        if ( boxes.size() == 2 )
        {
            queue.push(boxes[1]);
            n_colors++;
        }

        if ( n_colors >= target )
            return;
    }
}

} // namespace glaxnimate::utils::quantize::detail::mmcq

std::vector<QRgb> glaxnimate::utils::quantize::modified_median_cut(
    const QImage& image, int k,
    float fract_by_population,
    MatchType match,
    int max_iterations)
{

    using namespace glaxnimate::utils::quantize::detail::mmcq;

    auto histogram = color_frequencies(image);

    // Avoid processing if we don't need to
    if ( int(histogram.size()) <= k || k <= 2)
    {
        return detail::color_frequencies_to_palette(histogram, k);
    }

    if ( k > 256 )
        k = 256;

    auto vbox = VBox::from_histogram(&histogram);

    std::priority_queue<VBox, std::vector<VBox>, VBoxCompareCount> queue;
    queue.push(vbox);
    iterate(queue, &histogram, fract_by_population * k, max_iterations);

    std::priority_queue<VBox> queue2;
    while ( !queue.empty() )
    {
        queue2.push(queue.top());
        queue.pop();
    }

    iterate(queue2, &histogram, k - queue2.size(), max_iterations);

    std::vector<QRgb> result;
    result.reserve(queue2.size());
    while ( !queue2.empty() )
    {
        result.push_back(queue2.top().average(match));
        queue2.pop();
    }

    return result;
}
