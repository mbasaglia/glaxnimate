#include "segmentation.hpp"
#include <unordered_set>

#include <QDebug>

static const std::array<std::pair<int, int>, 4> orthogonal = {{
            {-1, 0},
    {0, -1},        {0, 1},
            {1, 0}
}};

static const std::array<std::pair<int, int>, 8> orthogonal_and_diagonal = {{
    {-1, -1}, {-1, 0},  {-1, 1},
    {0, -1},            {0, 1},
    {1, -1}, {1, 0},    {1, 1},
}};

int debug_segmentation = 0;

static void debug_cluster(const glaxnimate::trace::Cluster& cluster)
{
    auto db = qDebug();
    db << cluster.id << QColor::fromRgba(cluster.color).name() << cluster.size;
    if ( cluster.merge_target != glaxnimate::trace::Cluster::null_id )
        db << "->" << cluster.merge_target;
}

static void debug_clusters(const std::vector<glaxnimate::trace::Cluster>& clusters_)
{
    if ( debug_segmentation )
    {
        qDebug() << "[";
        for ( const auto& cluster : clusters_ )
            debug_cluster(cluster);
        qDebug() << "]";
    }
}

template<class T>
static void debug_vector(const std::vector<T>& v, int wrap = -1)
{
    if ( debug_segmentation )
    {
        int max = 0;
        for ( auto i : v )
            max = qMax(QString::number(i).size() + 1, max);

        int col = 0;
        QString row;
        for ( auto i : v )
        {
            row += QString::number(i).rightJustified(max) + " ";
            col += 1;
            if ( col == wrap )
            {
                col = 0;
                qDebug().noquote() << row;
                row = "";
            }
        }

        if ( !row.isEmpty() )
            qDebug().noquote() << row;
    }
}

static void debug_bitmap(const glaxnimate::trace::SegmentedImage& bitmap)
{
    debug_vector(bitmap.bitmap(), bitmap.width());

    if ( debug_segmentation )
        qDebug() << "-";
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
    debug_clusters(clusters_);

    // create a "map" id -> merge_target->id
    std::vector<int> mergers(clusters_.size() + 1, Cluster::null_id);
    for ( std::size_t i = 0; i < clusters_.size(); i++ )
    {
        if ( clusters_[i].merge_target != Cluster::null_id )
        {
            mergers[i + 1] = clusters_[i].merge_target;
            cluster(clusters_[i].merge_target)->size += clusters_[i].size;
        }
        else
        {
            mergers[i + 1] = clusters_[i].id;
        }
    }

    // Update ids
    update_cluster_ids(mergers, false);


    debug_bitmap(*this);
    debug_clusters(clusters_);

    // Remove merged clusters_ and map ID changes
    // Similar as remove_if but we keep track of IDs to update
    Cluster::id_type new_id = 0;
    std::vector<Cluster::id_type> new_ids;
    new_ids.reserve(clusters_.size()+1);
    new_ids.push_back(Cluster::null_id);

    auto iterator = clusters_.begin();
    auto end_iterator = clusters_.end();
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
        debug_clusters(clusters_);

        // Remove clusters_ merged into something else
        clusters_.erase(result_iterator, clusters_.end());
        debug_clusters(clusters_);

        // Update Ids
        update_cluster_ids(new_ids, true);

        debug_clusters(clusters_);
        debug_bitmap(*this);
    }

    if ( debug_segmentation )
        qDebug() << "===========";
}

glaxnimate::trace::Cluster::id_type glaxnimate::trace::SegmentedImage::add_cluster(QRgb color, int size)
{
    auto id = next_id();
    clusters_.push_back(Cluster{id, color, size});
    return id;
}


glaxnimate::trace::Cluster::id_type glaxnimate::trace::SegmentedImage::cluster_id(int x, int y) const
{
    if ( x < 0 || y < 0 || x >= width_ || y >= height_ )
        return Cluster::null_id;
    return bitmap_[x + y * width_];
}


glaxnimate::trace::Cluster* glaxnimate::trace::SegmentedImage::cluster(int x, int y)
{
    return cluster(cluster_id(x, y));
}


glaxnimate::trace::Cluster::id_type glaxnimate::trace::SegmentedImage::next_id() const
{
    return clusters_.size() + 1;
}

glaxnimate::trace::Cluster* glaxnimate::trace::SegmentedImage::cluster(Cluster::id_type id)
{
    if ( id == Cluster::null_id )
        return nullptr;
    return &clusters_[id - 1];
}

const glaxnimate::trace::Cluster * glaxnimate::trace::SegmentedImage::cluster(Cluster::id_type id) const
{
    if ( id == Cluster::null_id )
        return nullptr;
    return &clusters_[id - 1];
}


void glaxnimate::trace::SegmentedImage::unique_colors(bool flatten_alpha)
{
    std::unordered_map<QRgb, Cluster*> colors;
    for ( auto& cluster : clusters_ )
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
    for ( const auto& cluster : clusters_ )
        hist[cluster.color] += cluster.size;
    return hist;
}

QImage glaxnimate::trace::SegmentedImage::to_image() const
{
    QImage image(width_, height_, QImage::Format_ARGB32);
    auto pixels = reinterpret_cast<QRgb*>(image.bits());
    for ( std::size_t i = 0; i != bitmap_.size(); i++ )
    {
        if ( bitmap_[i] == Cluster::null_id )
            pixels[i] = 0;
        else
            pixels[i] = cluster(bitmap_[i])->color;
    }
    return image;
}

void glaxnimate::trace::SegmentedImage::update_cluster_ids(const std::vector<Cluster::id_type>& new_ids, bool update_ids)
{
//     auto old_holes_back = holes_back;
//     holes_back.clear();
//     for ( auto p : old_holes_back )
//     {
//         std::unordered_set<Cluster::id_type> set;
//         for ( auto& id : p.second )
//             set.insert(new_ids[id]);
//         holes_back.emplace(new_ids[p.first], set);
//     }
//
//     auto old_hole_wrapper = hole_wrapper;
//     hole_wrapper.clear();
//     for ( auto p : old_hole_wrapper )
//     {
//         hole_wrapper.emplace(new_ids[p.first], new_ids[p.second]);
//     }

    if ( update_ids )
    {
        for ( auto& cluster : clusters_ )
            cluster.id = new_ids[cluster.id];
    }

    for ( auto& pix : bitmap_ )
        pix = new_ids[pix];
}

void glaxnimate::trace::SegmentedImage::fix_cluster_ids()
{
    std::vector<Cluster::id_type> new_ids(clusters_.size() + 1, Cluster::null_id);
    for ( std::size_t i = 0; i < clusters_.size(); i++ )
        new_ids[clusters_[i].id] = i + 1;

    update_cluster_ids(new_ids, true);
}

void glaxnimate::trace::SegmentedImage::direct_merge(Cluster::id_type from, Cluster::id_type to)
{
    for ( auto & pix : bitmap_ )
        if ( pix == from )
            pix = to;

    cluster(to)->size += cluster(from)->size;
    clusters_.erase(clusters_.begin() + from - 1);
    fix_cluster_ids();
}

qint32 glaxnimate::trace::pixel_distance(QRgb p1, QRgb p2)
{
    int r1 = qRed(p1);
    int g1 = qGreen(p1);
    int b1 = qBlue(p1);
    int a1 = qAlpha(p1);

    int r2 = qRed(p2);
    int g2 = qGreen(p2);
    int b2 = qBlue(p2);
    int a2 = qAlpha(p2);

    qint32 dr = r1 - r2;
    qint32 dg = g1 - g2;
    qint32 db = b1 - b2;
    qint32 da = a1 - a2;

    return dr * dr + dg * dg + db * db + da * da;
}


qint32 glaxnimate::trace::closest_match(QRgb pixel, const std::vector<QRgb> &clut)
{
    int idx = 0;
    qint32 current_distance = 255 * 255 * 3;
    for ( std::size_t i = 0; i < clut.size(); ++i)
    {
        int dist = glaxnimate::trace::pixel_distance(pixel, clut[i]);
        if ( dist < current_distance )
        {
            current_distance = dist;
            idx = i;
        }
    }
    return idx;
}

namespace {

using namespace glaxnimate::trace;


struct Segmenter
{
    using Color = QRgb;

    Segmenter(SegmentedImage& segmented, const QImage& image, int alpha_threshold, bool diagonal_ajacency)
    : segmented(segmented), alpha_threshold(alpha_threshold), diagonal_ajacency(diagonal_ajacency), image(image)
    {
        if ( this->image.format() != QImage::Format_ARGB32 )
            this->image.convertTo(QImage::Format_ARGB32);
        pixels = reinterpret_cast<const QRgb*>(this->image.constBits());
    }

    Color pixel(int x, int y) const
    {
        if ( x < 0 || y < 0 )
            return 0;
        return pixels[y * segmented.width() + x];
    }

    // Hoshen–Kopelman algorithm but we also merge diagonals
    void process()
    {
        for ( int y = 0; y < segmented.height(); y++ )
        {
            for ( int x = 0; x < segmented.width(); x++ )
            {
                Color color = pixel(x, y);
                auto left = pixel(x - 1, y);
                auto up = pixel(x, y - 1);

                auto cluster_left = segmented.cluster(x - 1, y);
                auto cluster_up = segmented.cluster(x, y - 1);

                // Merge left and up clusters_ (they touch through a diagonal that isn't checked)
                if ( left == up && cluster_left != cluster_up && cluster_left && cluster_up
                    && (diagonal_ajacency || color == left)
                )
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
                    if ( diagonal_ajacency && diagonal == color && cluster_diagonal )
                    {
                        segmented.bitmap()[x + segmented.width() * y] = cluster_diagonal->id;
                        cluster_diagonal->size += 1;
                    }
                    // Create a new cluster
                    else
                    {
                        segmented.bitmap()[x + segmented.width() * y] = segmented.add_cluster(color);
                    }
                }
                // Neighbour to the left
                else if ( cluster_left )
                {
                    segmented.bitmap()[x + segmented.width() * y] = cluster_left->id;
                    cluster_left->size += 1;
                }
                // Neighbour above
                else if ( cluster_up )
                {
                    segmented.bitmap()[x + segmented.width() * y] = cluster_up->id;
                    cluster_up->size += 1;
                }
            }
        }
        debug_clusters(segmented.clusters());
        debug_bitmap(segmented);
        segmented.normalize();
    }

    SegmentedImage& segmented;
    const QRgb* pixels;
    int alpha_threshold;
    bool diagonal_ajacency;
    QImage image;
};

} // namespace

glaxnimate::trace::SegmentedImage glaxnimate::trace::segment(const QImage& image, int alpha_threshold, bool diagonal_ajacency)
{
    SegmentedImage segmented(image.width(), image.height());
    Segmenter segmenter(segmented, image, alpha_threshold, diagonal_ajacency);
    segmenter.process();
    return segmented;
}

void glaxnimate::trace::SegmentedImage::quantize(const std::vector<QRgb>& colors)
{
    auto min_id = next_id();

    for ( auto & cluster : clusters_ )
        cluster.merge_target = min_id + closest_match(cluster.color, colors);

    for ( auto color : colors )
        add_cluster(color, 0);

    normalize();
}

void glaxnimate::trace::SegmentedImage::dilate(Cluster::id_type id, int protect_size)
{
    auto source = cluster(id);
    std::vector<int> subtract(clusters_.size() + 1, 0);

    debug_bitmap(*this);

    for ( int y = 0; y < height_; y++ )
    {
        for ( int x = 0; x < width_; x++ )
        {
            if ( cluster_id(x, y) == id )
            {
                if ( debug_segmentation )
                    qDebug() << x << y << ":";

                for ( auto d : orthogonal_and_diagonal )
                {
                    int fy = y + d.first;
                    int fx = x + d.second;
                    if ( fx < 0 || fy < 0 || fx >= width_ || fy >= height_ )
                        continue;

                    auto& pix = bitmap_[fx + fy * width_];
                    if ( pix > 0 && pix != id && (protect_size < 0 || cluster(pix)->size < protect_size) )
                    {
                        subtract[pix] += 1;
                        pix = -id;
                    }
                }

                debug_vector(subtract);
                debug_bitmap(*this);
            }
        }
    }

    for ( auto & pix : bitmap_ )
        if ( pix < 0 )
            pix = id;

    debug_clusters(clusters_);
    debug_vector(subtract);
    debug_bitmap(*this);

    for ( std::size_t i = 0; i < clusters_.size(); i++ )
    {
        clusters_[i].size -= subtract[i+1];
        source->size += subtract[i+1];
    }

    debug_clusters(clusters_);
    if ( debug_segmentation )
        qDebug() << "======";
}

int glaxnimate::trace::SegmentedImage::perimeter(glaxnimate::trace::Cluster::id_type id) const
{
    int perimeter = 0;

    for ( int y = 0; y < height_; y++ )
    {
        for ( int x = 0; x < width_; x++ )
        {
            if ( cluster_id(x, y) == id )
            {
                for ( auto d : orthogonal )
                {
                    if ( cluster_id(x + d.second, y + d.first) != id )
                    {
                        perimeter += 1;
                        break;
                    }
                }
            }
        }
    }

    return perimeter;
}

std::vector<Cluster::id_type> glaxnimate::trace::SegmentedImage::neighbours(Cluster::id_type id) const
{
    std::unordered_set<Cluster::id_type> neighbours;
    for ( int y = 0; y < height_; y++ )
    {
        for ( int x = 0; x < width_; x++ )
        {
            if ( cluster_id(x, y) == id )
            {
                for ( auto d : orthogonal )
                {
                    auto neigh = cluster_id(x + d.second, y + d.first);
                    if ( neigh != id )
                        neighbours.insert(neigh);
                }
            }
        }
    }

    neighbours.erase(id);
    return std::vector<Cluster::id_type>(neighbours.begin(), neighbours.end());
}
/*
void glaxnimate::trace::SegmentedImage::add_hole(Cluster::id_type hole, Cluster::id_type container)
{
    holes_back[container].insert(hole);
    hole_wrapper[hole] = container;
}

glaxnimate::trace::Cluster::id_type glaxnimate::trace::SegmentedImage::hole_parent(Cluster::id_type cluster) const
{
    auto it = hole_wrapper.find(cluster);
    if ( it == hole_wrapper.end() )
        return Cluster::null_id;
    return it->second;
}*/
