#include "quantize.hpp"

#include <unordered_map>
#include <memory>

#include "math/math.hpp"

using namespace glaxnimate;

namespace glaxnimate::trace::detail {

static bool freq_sort_cmp(const ColorFrequency& a, const ColorFrequency& b) noexcept
{
    return a.second > b.second;
}

std::vector<QRgb> color_frequencies_to_palette(std::vector<trace::ColorFrequency>& freq, int max)
{
    std::sort(freq.begin(), freq.end(), detail::freq_sort_cmp);

    int count = qMin<int>(max, freq.size());
    std::vector<QRgb> out;
    out.reserve(count);

    for ( int i = 0; i < count; i++ )
        out.push_back(freq[i].first);

    return out;
}

} // trace::detail

std::vector<trace::ColorFrequency> trace::color_frequencies(const SegmentedImage& image)
{
    auto count = image.histogram();
    return std::vector<ColorFrequency>(count.begin(), count.end());
}


std::vector<QRgb> trace::k_modes(SegmentedImage& image, int k)
{
    auto freq = color_frequencies(image);
    return detail::color_frequencies_to_palette(freq, k);
}


namespace glaxnimate::trace::detail::k_means {

struct Point
{
    StructuredColor color;

    quint32 weight;

    ColorDistance min_distance = std::numeric_limits<ColorDistance>::max();

    int cluster = -1;


    constexpr Point(const ColorFrequency& p) noexcept
        : color(p.first), weight(p.second)
    {}
};


struct Cluster
{
    StructuredColor centroid;
    quint32 total_weight = 0;
    StructuredColor sum = {};

    constexpr Cluster(const StructuredColor& color) noexcept
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

} // trace::detail

std::vector<QRgb> trace::k_means(SegmentedImage& image, int k, int iterations, KMeansMatch match)
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
        ColorDistance max_dist = 0;

        for ( auto it = points.begin(); it != points.end(); ++it )
        {
            ColorDistance p_max = 0;
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
            p.min_distance = std::numeric_limits<ColorDistance>::max();
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
                std::numeric_limits<ColorDistance>::max() - p.color.distance(cluster.centroid);

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
namespace glaxnimate::trace::detail::octree {


inline StructuredColor operator>>(StructuredColor rgb, int s)
{
    StructuredColor res;
    res.r = rgb.r >> s; res.g = rgb.g >> s; res.b = rgb.b >> s;
    return res;
}

inline int childIndex(StructuredColor rgb)
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
    StructuredColor rgb;
    // number of pixels this node accounts for
    quint32 weight = 0;
    // sum of pixels colors this node accounts for
    StructuredColor sum;
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
static std::unique_ptr<Node> ocnodeLeaf(StructuredColor rgb, quint32 weight)
{
    auto node = std::make_unique<Node>();
    node->width = 0;
    node->rgb = rgb;
    node->sum = StructuredColor(rgb.r * weight, rgb.g * weight, rgb.b * weight);
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
            StructuredColor rgb1 = node1->rgb >> (newwidth - node1->width);
            StructuredColor rgb2 = node2->rgb >> (newwidth - node2->width);
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

} // namespace glaxnimate::trace::detail::octree


std::vector<QRgb> trace::octree(SegmentedImage& image, int k)
{
    using namespace glaxnimate::trace::detail::octree;

    auto freq = color_frequencies(image);

    // Avoid processing if we don't need to
    if ( int(freq.size()) <= k || k <= 1)
        return detail::color_frequencies_to_palette(freq, k);

    std::vector<QRgb> colors;
    colors.reserve(k);


    std::unique_ptr<Node> tree = add_pixels(nullptr, freq.data(), freq.size());
    tree = octreePrune(std::move(tree), k);

    tree->get_colors(colors);

    return colors;
}

namespace glaxnimate::trace::detail::auto_colors {

void decrease(QRgb color, Histogram& map)
{
    auto it = map.find(color | 0xff000000u);
    if ( it != map.end() )
        it->second -= 1;
}

} // namespace glaxnimate::trace::detail::auto_colors

std::vector<QRgb> trace::edge_exclusion_modes(SegmentedImage& image, int max_colors, int min_area)
{
    image.unique_colors();

    std::vector<Cluster*> clusters;
    clusters.reserve(image.size());
    for ( auto& cluster: image )
        clusters.push_back(&cluster);

    std::sort(clusters.begin(), clusters.end(), [](Cluster* a, Cluster* b) {
        return a->size > b->size;
    });

    std::size_t iter = 0;
    for ( iter = 0; iter < clusters.size() && iter <= std::size_t(max_colors); ++iter )
    {
        int largest = 0;
        std::size_t largest_off = clusters.size();
        for ( auto find_it = iter; find_it != clusters.size(); ++find_it )
        {
            if ( clusters[find_it]->size > largest )
            {
                largest = clusters[find_it]->size;
                largest_off = find_it;
            }
        }

        if ( largest <= min_area || largest_off == clusters.size() )
        {
            break;
        }

        image.dilate(clusters[largest_off]->id, min_area);

        if ( largest_off != iter )
            std::swap(clusters[iter], clusters[largest_off]);
    }

    auto freq = color_frequencies(image);
    return detail::color_frequencies_to_palette(freq, max_colors);
}

namespace std {
    template<>
    struct hash<std::pair<int, int>>
    {
        std::size_t operator()(std::pair<int, int> const &p) const
        {
            std::size_t h1 = std::hash<int>{}(p.first);
            std::size_t h2 = std::hash<int>{}(p.second);
            return h1 ^ (h2 << 1);
        }
    };
}

namespace glaxnimate::trace::detail::cluster_merge {

bool large_enough(Cluster* cluster, const SegmentedImage& image, int min_area)
{
    if ( cluster->size < min_area )
        return false;

    if ( cluster->size == image.perimeter(cluster->id) )
        return false;

    return true;
}

// int get_hole_depth(Cluster* cluster, SegmentedImage& image, std::vector<int>& hole_depth)
// {
//     auto holed = image.cluster(image.hole_parent(cluster->id));
//
//     if ( holed && hole_depth[holed->id] == -1 )
//         return hole_depth[cluster->id] = get_hole_depth(holed, image, hole_depth);
//     else
//         return hole_depth[cluster->id] = 0;
// }
void find_gradient(Cluster& cluster, SegmentedImage& image, BrushData& result, int min_color_distance, Cluster::id_type max_id)
{
    std::unordered_set<Cluster::id_type> matching(cluster.merge_sources.begin(), cluster.merge_sources.end());
    matching.insert(cluster.id);
    std::unordered_map<std::pair<int, int>, StructuredColor> points;
    int min_x = 0;
    int max_x = 0;
    int min_y = 0;
    int max_y = 0;

    int largest_min_x = image.width();
    int largest_min_y = image.height();
    int largest_max_x = 0;
    int largest_max_y = 0;

    for ( int y = 0; y < image.height(); y++ )
    {
        for ( int x = 0; x < image.width(); x++ )
        {
            auto id = image.cluster_id(x, y);
            if ( matching.count(id) )
            {
                if ( points.empty() )
                {
                    min_x = x;
                    min_y = y;
                }
                max_x = x;
                max_y = y;

                if ( id == max_id )
                {
                    if ( x > largest_max_x )
                        largest_max_x = x;
                    if ( x < largest_min_x )
                        largest_min_x = x;
                    if ( y > largest_max_y )
                        largest_max_y = x;
                    if ( y < largest_min_y )
                        largest_min_y = x;
                }

                points.insert({{x, y}, image.cluster(id)->color});
            }
        }
    }

    float center_x = (min_x + max_x) / 2.;
    float center_y = (min_y + max_y) / 2.;
    QGradientStops stops;
    QPoint p1;
    QPoint p2;

    float angle = math::atan2(largest_max_y - largest_min_y, largest_max_x - largest_min_x) - math::pi / 2;

    std::vector<std::pair<int, int>> line;
    // TODO
    // get line of pixels -- https://en.wikipedia.org/wiki/Bresenham's_line_algorithm
    if ( line.size() < 3 )
        return;

    ColorDistance total_distance = 0;
    int pixel_count = 0;
    std::size_t previous_index = 0;
    StructuredColor previous_color = 0;
    std::vector<StructuredColor> colors;

    for ( std::size_t i = 0; i < line.size(); i++ )
    {
        auto it = points.find(line[i]);
        if ( it == points.end() )
            continue;

        auto color = it->second;
        if ( !colors.empty() )
            total_distance += previous_color.distance(color);
        pixel_count++;
        colors.push_back(color);
        float delta = i - previous_index;
        if ( delta > 1 )
        {
            for ( std::size_t j = 1; j < delta; j++ )
                colors.push_back(previous_color.lerp(color, j / delta));
        }
        previous_color = color;
        previous_index = i;

    }

    if ( pixel_count < 3 )
        return;

    float mean_distance = total_distance / float(previous_index);

    if ( mean_distance > min_color_distance * 2 )
    {
        QLinearGradient gradient;
        gradient.setStart(p1);
        gradient.setFinalStop(p2);
        gradient.setStops(gradient_stops(colors));
        result.gradients.emplace(cluster.id, gradient);
    }
}

void get_cluster_result(Cluster& cluster, SegmentedImage& image, BrushData& result, int min_color_distance)
{
    int total_size = cluster.size;
    int max_size = cluster.size;
    QRgb color = cluster.color;
    Cluster::id_type max_id = cluster.id;
    for ( auto merged_id : cluster.merge_sources )
    {
        auto merged = image.cluster(merged_id);
        total_size += merged->size;
        if ( merged->size > max_size )
        {
            max_id = merged_id;
            max_size = merged->size;
            color = merged->color;
        }
    }

    cluster.color = color;

    // Probably a gradient
    // using total_size > 100 to have something like a 10x10 pixels minimum
    // it might be better to base this on the image size rather than being fixed
    if ( cluster.size < total_size / 2 && total_size > 100 && cluster.merge_sources.size() > 10 )
        find_gradient(cluster, image, result, min_color_distance, max_id);
}


} // namespace glaxnimate::trace::detail::cluster_merge


glaxnimate::trace::BrushData glaxnimate::trace::cluster_merge(glaxnimate::trace::SegmentedImage& image,
                                                   int max_colors, int min_area, int min_color_distance)
{
    using namespace glaxnimate::trace::detail::cluster_merge;

    // For each cluster, starting with the smallest size
    for ( Cluster& cluster : image )
    {
        // Find the most similar neighbour with area less than the current cluster
        ColorDistance min_distance = std::numeric_limits<ColorDistance>::max();
        int min_size = std::numeric_limits<int>::max();
        Cluster* similar_neighbour = nullptr;
        auto neighbours = image.neighbours(cluster.id);
        for ( auto neigh_id : neighbours )
        {
            auto neigh = image.cluster(neigh_id);

            if ( neigh->merge_target == cluster.id )
                continue;

            if ( neigh->merge_target )
                neigh = image.cluster(neigh->merge_target);

            // if ( hole_of[cluster->id] != neigh )
            auto distance = rgba_distance_squared(cluster.color, neigh->color);
            if ( distance <= min_distance || (distance == min_distance && neigh->size < min_size) )
            {
                similar_neighbour = neigh;
                min_distance = distance;
                min_size = neigh->size;
            }
        }

        if ( !similar_neighbour )
            continue;

        // If the current cluster is not large enough, merge it to the similar neighbour
        if ( (min_distance <= min_color_distance && min_size > cluster.size) ||
                !large_enough(&cluster, image, min_area) )
        {
            image.merge(&cluster, similar_neighbour);
        }
//         else
//         {
//             // is hole
//             if ( neighbours.size() == 1 )
//                 image.add_hole(cluster->id, similar_neighbour);
//         }
    }
/*
    std::vector<int> hole_depth(clusters.size() + 1, -1);
    for ( auto cluster : clusters )
        get_hole_depth(cluster, image, hole_depth);

    image.sort_clusters([&hole_depth](const Cluster& a, const Cluster& b) {
        return hole_depth[a.id] < hole_depth[b.id];
    });*/

    BrushData result;

    for ( auto& cluster : image )
    {
        if ( !cluster.merge_sources.empty() )
            get_cluster_result(cluster, image, result, min_color_distance);
    }

    // apply merges
    image.normalize();

    // Limit colors to max_colors
    result.colors = k_modes(image, std::numeric_limits<int>::max());
    if ( int(result.colors.size()) > max_colors )
    {
        std::vector<QRgb> tail(result.colors.begin() + max_colors, result.colors.end());
        result.colors.erase(result.colors.begin() + max_colors, result.colors.end());
        std::unordered_map<QRgb, QRgb> replacements;
        for ( auto col : tail )
            replacements[col] = result.colors[closest_match(col, result.colors)];
        for ( auto& col : result.colors )
            replacements[col] = col;

        for ( auto& cluster : image )
            cluster.color = replacements[cluster.color];
    }

    return result;
}
