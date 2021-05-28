#include "quantize.hpp"


std::vector<std::pair<QRgb, int>> utils::quantize::color_frequencies(QImage image, int alpha_threshold)
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


using Distance = quint32;

struct Color
{
    qint32 r;
    qint32 g;
    qint32 b;

    constexpr Color(QRgb rgb = 0) noexcept
    : r(qRed(rgb)), g(qGreen(rgb)), b(qBlue(rgb))
    {}

    Distance distance(const Color& oth) const noexcept
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
};

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
        detail::k_means::Distance max_dist = 0;

        for ( auto it = points.begin(); it != points.end(); ++it )
        {
            detail::k_means::Distance p_max = 0;
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
            clusters[p.cluster].sum.r += p.weight * p.color.r;
            clusters[p.cluster].sum.g += p.weight * p.color.g;
            clusters[p.cluster].sum.b += p.weight * p.color.b;
            p.min_distance = std::numeric_limits<detail::k_means::Distance>::max();
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
