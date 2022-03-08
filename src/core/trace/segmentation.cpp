#include "segmentation.hpp"

#include <QDebug>

int debug_segmentation = 0;

static void debug_cluster(const glaxnimate::trace::Cluster& cluster)
{
    auto db = qDebug();
    db << cluster.id << QColor::fromRgba(cluster.color).name() << cluster.size;
    if ( cluster.merge_target != glaxnimate::trace::Cluster::null_id )
        db << "->" << cluster.merge_target;
}

static void debug_clusters(const std::vector<glaxnimate::trace::Cluster>& clusters)
{
    if ( debug_segmentation )
    {
        qDebug() << "[";
        for ( const auto& cluster : clusters )
            debug_cluster(cluster);
        qDebug() << "]";
    }
}

static void debug_bitmap(const glaxnimate::trace::SegmentedImage& bitmap)
{
    if ( debug_segmentation )
    {
        int max = QString::number(bitmap.next_id()).size();
        for ( int y = 0; y < bitmap.height; y++ )
        {
            QString row;
            for ( int x = 0; x < bitmap.width; x++ )
                row += QString::number(bitmap.cluster_id(x, y)).leftJustified(max);
            qDebug() << row;
        }
        qDebug() << "-";
    }
}


bool glaxnimate::trace::Cluster::is_valid() const
{
    return merge_target == null_id && size > 0;
}

void glaxnimate::trace::SegmentedImage::merge(Cluster* from, Cluster* to)
{
    if ( to->merge_target != Cluster::null_id )
        to = cluster(to->merge_target);

    if ( from == to || to->id == from->merge_target )
        return;

    Cluster* merge_parent = from;

    if ( from->merge_target != Cluster::null_id )
        merge_parent = cluster(from->merge_target);

    merge_parent->merge_target = to->id;
    to->merge_sources.push_back(merge_parent->id);
    to->merge_sources.insert(to->merge_sources.end(), merge_parent->merge_sources.begin(), merge_parent->merge_sources.end());
    for ( auto id : merge_parent->merge_sources )
        cluster(id)->merge_target = to->id;
    merge_parent->merge_sources.clear();
}


void glaxnimate::trace::SegmentedImage::normalize()
{
    if ( debug_segmentation )
        qDebug() << "Normalize";
    debug_bitmap(*this);
    debug_clusters(clusters);

    // create a "map" id -> merge_target->id
    std::vector<int> mergers(clusters.size() + 1, Cluster::null_id);
    for ( std::size_t i = 0; i < clusters.size(); i++ )
    {
        if ( clusters[i].merge_target != Cluster::null_id )
        {
            mergers[i + 1] = clusters[i].merge_target;
            cluster(clusters[i].merge_target)->size += clusters[i].size;
        }
        else
        {
            mergers[i + 1] = clusters[i].id;
        }
    }

    // Update ids
    for ( auto& pix : bitmap )
        pix = mergers[pix];


    debug_bitmap(*this);
    debug_clusters(clusters);

    // Remove merged clusters and map ID changes
    // Similar as remove_if but we keep track of IDs to update
    Cluster::id_type new_id = 0;
    std::vector<Cluster::id_type> new_ids;
    new_ids.reserve(clusters.size()+1);
    new_ids.push_back(Cluster::null_id);

    auto iterator = clusters.begin();
    auto end_iterator = clusters.end();
    for ( ; iterator != end_iterator && iterator->is_valid(); ++iterator )
    {
        iterator->merge_sources.clear();
        new_ids.push_back(++new_id);
    }

    if ( iterator != end_iterator )
    {
        auto result_iterator = iterator;
        new_ids.push_back(Cluster::null_id);
        for ( ++iterator; iterator != end_iterator; ++iterator)
        {
            if ( iterator->is_valid() )
            {
                iterator->merge_sources.clear();
                *result_iterator = std::move(*iterator);
                ++result_iterator;
                new_ids.push_back(++new_id);
            }
            else
            {
                new_ids.push_back(Cluster::null_id);
            }
        }
        debug_clusters(clusters);

        // Remove clusters merged into something else
        clusters.erase(result_iterator, clusters.end());
        debug_clusters(clusters);

        // Update Ids
        for ( auto& cluster : clusters )
            cluster.id = new_ids[cluster.id];

        debug_clusters(clusters);

        for ( auto& pix : bitmap )
            pix = new_ids[pix];

        debug_bitmap(*this);
    }

    if ( debug_segmentation )
        qDebug() << "===========";
}

glaxnimate::trace::Cluster::id_type glaxnimate::trace::SegmentedImage::add_cluster(QRgb color, int size)
{
    auto id = next_id();
    clusters.push_back(Cluster{id, color, size});
    return id;
}


glaxnimate::trace::Cluster::id_type glaxnimate::trace::SegmentedImage::cluster_id(int x, int y) const
{
    if ( x < 0 || y < 0 )
        return Cluster::null_id;
    return bitmap[x + y * width];
}


glaxnimate::trace::Cluster* glaxnimate::trace::SegmentedImage::cluster(int x, int y)
{
    return cluster(cluster_id(x, y));
}


glaxnimate::trace::Cluster::id_type glaxnimate::trace::SegmentedImage::next_id() const
{
    return clusters.size() + 1;
}

glaxnimate::trace::Cluster* glaxnimate::trace::SegmentedImage::cluster(Cluster::id_type id)
{
    if ( id == Cluster::null_id )
        return nullptr;
    return &clusters[id - 1];
}

void glaxnimate::trace::SegmentedImage::unique_colors(bool flatten_alpha)
{
    std::unordered_map<QRgb, Cluster*> colors;
    for ( auto& cluster : clusters )
    {
        if ( flatten_alpha )
            cluster.color |= 0xff000000u;

        auto& parent = colors[cluster.color];
        if ( parent )
            merge(&cluster, parent);
        else
            parent = &cluster;
    }

    normalize();
}

glaxnimate::trace::Histogram glaxnimate::trace::SegmentedImage::histogram() const
{
    glaxnimate::trace::Histogram hist;
    for ( const auto& cluster : clusters )
        hist[cluster.color] += cluster.size;
    return hist;
}



namespace {

using namespace glaxnimate::trace;

struct Segmenter
{
    using Color = QRgb;

    Segmenter(SegmentedImage& segmented, const QImage& image, int alpha_threshold)
    : segmented(segmented), alpha_threshold(alpha_threshold), image(image)
    {
        if ( this->image.format() != QImage::Format_ARGB32 )
            this->image.convertTo(QImage::Format_ARGB32);
        pixels = reinterpret_cast<const QRgb*>(this->image.constBits());
    }

    Color pixel(int x, int y) const
    {
        if ( x < 0 || y < 0 )
            return 0;
        return pixels[y * segmented.width + x];
    }

    // Hoshen–Kopelman algorithm but we also merge diagonals
    void process()
    {
        for ( int y = 0; y < segmented.height; y++ )
        {
            for ( int x = 0; x < segmented.width; x++ )
            {
                Color color = pixel(x, y);
                auto left = pixel(x - 1, y);
                auto up = pixel(x, y - 1);

                auto cluster_left = segmented.cluster(x - 1, y);
                auto cluster_up = segmented.cluster(x, y - 1);

                // Merge left and up clusters (they touch through a diagonal that isn't checked)
                if ( left == up && cluster_left != cluster_up && cluster_left && cluster_up )
                {
                    segmented.merge(cluster_up, cluster_left);
                }

                if ( qAlpha(color) < alpha_threshold )
                    continue;

                // Set to nullptr to avoid issues when a pixel adjacent to an edge is 0
                if ( left != color )
                    cluster_left = nullptr;
                if ( up != color )
                    cluster_up = nullptr;

                // No orthogonal neighbour
                if ( !cluster_left && !cluster_up )
                {
                    // Check if the current pixel should be added to the top-left cluster
                    auto diagonal = pixel(x - 1, y - 1);
                    auto cluster_diagonal = segmented.cluster(x - 1, y - 1);
                    if ( diagonal == color && cluster_diagonal )
                    {
                        segmented.bitmap[x + segmented.width * y] = cluster_diagonal->id;
                        cluster_diagonal->size += 1;
                    }
                    // Create a new cluster
                    else
                    {
                        segmented.bitmap[x + segmented.width * y] = segmented.add_cluster(color);
                    }
                }
                // Neighbour to the left
                else if ( cluster_left )
                {
                    segmented.bitmap[x + segmented.width * y] = cluster_left->id;
                    cluster_left->size += 1;
                }
                // Neighbour above
                else if ( cluster_up )
                {
                    segmented.bitmap[x + segmented.width * y] = cluster_up->id;
                    cluster_up->size += 1;
                }
            }
        }
        debug_clusters(segmented.clusters);
        debug_bitmap(segmented);
        segmented.normalize();
    }

    SegmentedImage& segmented;
    const QRgb* pixels;
    int alpha_threshold;
    QImage image;
};

} // namespace

glaxnimate::trace::SegmentedImage glaxnimate::trace::segment(const QImage& image, int alpha_threshold)
{
    SegmentedImage segmented(image.width(), image.height());
    Segmenter segmenter(segmented, image, alpha_threshold);
    segmenter.process();
    return segmented;
}
