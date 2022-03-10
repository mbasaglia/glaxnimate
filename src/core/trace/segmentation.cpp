#include "segmentation.hpp"

#include <unordered_set>

#include "utils/color.hpp"

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
    // create a "map" id -> merge_target->id
    std::unordered_map<Cluster::id_type, Cluster::id_type> mergers;
    for ( auto it = clusters_.begin(); it != clusters_.end(); )
    {
        if ( it->second.merge_target != Cluster::null_id )
        {
            mergers[it->second.id] = it->second.merge_target;
            cluster(it->second.merge_target)->size += it->second.size;
            it = clusters_.erase(it);
        }
        else
        {
            mergers[it->second.id] = it->second.id;
            it->second.merge_sources.clear();
            ++it;
        }
    }

    for ( auto it = clusters_.begin(); it != clusters_.end(); )
    {
        if ( it->second.size == 0 )
            it = clusters_.erase(it);
        else
            ++it;
    }

    for ( auto& pix : bitmap_ )
        pix = mergers[pix];
}

glaxnimate::trace::Cluster* glaxnimate::trace::SegmentedImage::add_cluster(QRgb color, int size)
{
    auto id = next_id++;
    return &clusters_.emplace(id, Cluster{id, color, size}).first->second;
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


glaxnimate::trace::Cluster* glaxnimate::trace::SegmentedImage::cluster(Cluster::id_type id)
{
    if ( id == Cluster::null_id )
        return nullptr;
    return &clusters_[id];
}

const glaxnimate::trace::Cluster * glaxnimate::trace::SegmentedImage::cluster(Cluster::id_type id) const
{
    if ( id == Cluster::null_id )
        return nullptr;
    return &clusters_.at(id);
}


void glaxnimate::trace::SegmentedImage::unique_colors(bool flatten_alpha)
{
    std::unordered_map<QRgb, Cluster*> colors;
    for ( auto& cluster : clusters_ )
    {
        if ( flatten_alpha )
            cluster.second.color |= 0xff000000u;

        auto& parent = colors[cluster.second.color];
        if ( parent )
        {
            // keep the one with lowest ID
            if ( parent->id < cluster.second.id )
            {
                merge(&cluster.second, parent);
            }
            else
            {
                merge(parent, &cluster.second);
                parent = &cluster.second;
            }
        }
        else
        {
            parent = &cluster.second;
        }
    }

    normalize();
}

glaxnimate::trace::Histogram glaxnimate::trace::SegmentedImage::histogram() const
{
    glaxnimate::trace::Histogram hist;
    for ( const auto& cluster : clusters_ )
        hist[cluster.second.color] += cluster.second.size;
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

void glaxnimate::trace::SegmentedImage::direct_merge(Cluster::id_type from, Cluster::id_type to)
{
    for ( auto & pix : bitmap_ )
        if ( pix == from )
            pix = to;

    cluster(to)->size += cluster(from)->size;
    clusters_.erase(clusters_.find(from));
}

qint32 glaxnimate::trace::closest_match(QRgb pixel, const std::vector<QRgb> &clut)
{
    int idx = 0;
    qint32 current_distance = 255 * 255 * 3;
    for ( std::size_t i = 0; i < clut.size(); ++i)
    {
        int dist = utils::color::rgba_distance_squared(pixel, clut[i]);
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

    Segmenter(SegmentedImage& segmented, const QImage& image, bool diagonal_ajacency)
    : segmented(segmented), diagonal_ajacency(diagonal_ajacency), image(image)
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
                    segmented.merge(cluster_left, cluster_up);
                }

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
                        segmented.bitmap()[x + segmented.width() * y] = segmented.add_cluster(color)->id;
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
        segmented.normalize();
    }

    SegmentedImage& segmented;
    const QRgb* pixels;
    bool diagonal_ajacency;
    QImage image;
};

} // namespace

glaxnimate::trace::SegmentedImage glaxnimate::trace::segment(const QImage& image, bool diagonal_ajacency)
{
    SegmentedImage segmented(image.width(), image.height());
    Segmenter segmenter(segmented, image, diagonal_ajacency);
    segmenter.process();
    return segmented;
}

void glaxnimate::trace::SegmentedImage::quantize(const std::vector<QRgb>& colors)
{
    auto min_id = next_id;

    for ( auto & cluster : clusters_ )
        cluster.second.merge_target = min_id + closest_match(cluster.second.color, colors);

    for ( auto color : colors )
        add_cluster(color, 0);

    normalize();
}

void glaxnimate::trace::SegmentedImage::dilate(Cluster::id_type id, int protect_size)
{
    auto source = cluster(id);
    std::map<Cluster::id_type, int> subtract;

    for ( int y = 0; y < height_; y++ )
    {
        for ( int x = 0; x < width_; x++ )
        {
            if ( cluster_id(x, y) == id )
            {
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
            }
        }
    }

    for ( auto& pix : bitmap_ )
        if ( pix < 0 )
            pix = id;

    for ( const auto& sub : subtract )
    {
        auto sub_cluster = cluster(sub.first);
        sub_cluster->size -= sub.second;
        source->size += sub.second;
    }
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
                    if ( neigh != id && neigh != Cluster::null_id )
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
