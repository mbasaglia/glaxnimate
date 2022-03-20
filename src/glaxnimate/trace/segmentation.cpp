#include "segmentation.hpp"

#include <unordered_set>

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


QString glaxnimate::trace::Cluster::to_string() const
{
    return QString("Cluster { id = %1 color = %2 size = %3 merge_target = %4 index = (%5,%6)}")
        .arg(id)
        .arg("0x" + QString::number(color, 16).rightJustified(8, '0'))
        .arg(size)
        .arg(merge_target)
        .arg(index_start)
        .arg(index_end)
    ;
}

QDebug operator<< (QDebug db, const glaxnimate::trace::Cluster& cluster)
{
    QDebugStateSaver saver(db);
    db.noquote() << cluster.to_string();
    return db;
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
    // create a "map" id -> merge_target->id
    std::vector<Cluster::id_type> mergers;
    mergers.resize(clusters_.size());

    for ( auto& it : clusters_ )
    {
        if ( !it )
            continue;

        if ( it->merge_target != Cluster::null_id )
        {
            mergers[it->id] = it->merge_target;
            auto target = cluster(it->merge_target);
            merge_attributes(&*it, target);
            do_erase(it);
        }
        else
        {
            mergers[it->id] = it->id;
            it->merge_sources.clear();
        }
    }

    for ( auto& ptr : clusters_ )
    {
        if ( ptr && ptr->size == 0 )
            do_erase(ptr);
    }

    for ( auto& pix : bitmap_ )
        pix = mergers[pix];
}

void glaxnimate::trace::SegmentedImage::merge_attributes(glaxnimate::trace::Cluster* from, glaxnimate::trace::Cluster* to)
{
    to->size += from->size;

    if ( from->index_start < to->index_start )
        to->index_start = from->index_start;

    if ( from->index_end > to->index_end )
        to->index_end = from->index_end;
}


glaxnimate::trace::Cluster* glaxnimate::trace::SegmentedImage::add_cluster(QRgb color, int size)
{
    auto id = next_id++;
    clusters_.push_back(std::make_unique<Cluster>(Cluster{id, color, size}));
    size_++;
    return clusters_.back().get();
}


void glaxnimate::trace::SegmentedImage::unique_colors(bool flatten_alpha)
{
    std::unordered_map<QRgb, Cluster*> colors;
    for ( auto& cluster : *this )
    {
        if ( flatten_alpha )
            cluster.color |= 0xff000000u;

        auto& parent = colors[cluster.color];
        if ( parent )
        {
            merge(&cluster, parent);
        }
        else
        {
            parent = &cluster;
        }
    }

    normalize();
}

glaxnimate::trace::Histogram glaxnimate::trace::SegmentedImage::histogram(bool flatten_alpha) const
{
    glaxnimate::trace::Histogram hist;
    if ( flatten_alpha )
    {
        for ( const auto& cluster : *this )
            hist[cluster.color] += cluster.size;
    }
    else
    {
        for ( const auto& cluster : *this )
            hist[cluster.color|0xff000000u] += cluster.size;
    }
    return hist;
}

static QRgb unique_color(int id)
{
    int hue = id * 67 % 360;
    int saturation = 255;
    int value = 255;
    switch ( id / 360 % 4 )
    {
        case 0:
            saturation = 255;
            value = 255;
            break;
        case 1:
            saturation = 128;
            value = 255;
            break;
        case 2:
            saturation = 128;
            value = 128;
            break;
        case 3:
            saturation = 255;
            value = 128;
            break;
    }

    return QColor::fromHsv(hue, saturation, value).rgba();
}

QImage glaxnimate::trace::SegmentedImage::to_image(bool merged, bool debug) const
{
    QImage image(width_, height_, QImage::Format_ARGB32);
    auto pixels = reinterpret_cast<quint32*>(image.bits());
    for ( std::size_t i = 0; i != bitmap_.size(); i++ )
    {
        if ( bitmap_[i] == Cluster::null_id )
        {
            pixels[i] = 0;
        }
        else
        {
            auto cluster = this->cluster(bitmap_[i]);
            if ( merged && cluster->merge_target != Cluster::null_id )
                cluster = this->cluster(cluster->merge_target);
            pixels[i] = debug ? unique_color(cluster->id) : cluster->color;
        }
    }
    return image;
}

void glaxnimate::trace::SegmentedImage::direct_merge(Cluster::id_type from, Cluster::id_type to)
{
    auto from_cluster = cluster(from);

    for ( auto & pix : pixel_range(from_cluster) )
        if ( pix == from )
            pix = to;

    merge_attributes(cluster(from), cluster(to));

    do_erase(clusters_[from]);
}


// Hoshen–Kopelman algorithm but we also merge diagonals
void glaxnimate::trace::SegmentedImage::segment(const quint32* pixels, bool diagonal_ajacency)
{
    clusters_.reserve(bitmap_.size() / 4);
    for ( int y = 0; y < height_; y++ )
    {
        Cluster* old_clust = nullptr;
        for ( int x = 0; x < width_; x++ )
        {
            auto index = x + width_ * y;
            QRgb color = pixels[index];

            auto cluster_left = old_clust;
            auto cluster_up = y > 0 ? cluster_by_offset(index - width_) : nullptr;

            // Merge left and up clusters_ (they touch through a diagonal that isn't checked)
            if ( cluster_left && cluster_up && cluster_left != cluster_up
                && cluster_left->color == cluster_up->color
                && (diagonal_ajacency || color == cluster_left->color)
            )
            {
                merge(cluster_left, cluster_up);
                cluster_left = nullptr;
            }

            // Set to nullptr to avoid issues when a pixel adjacent to an edge is 0
            if ( cluster_left && cluster_left->color != color )
                cluster_left = nullptr;
            if ( cluster_up && cluster_up->color != color )
                cluster_up = nullptr;
            // No orthogonal neighbour
            if ( !cluster_left && !cluster_up )
            {
                // Check if the current pixel should be added to the top-left cluster
                auto cluster_diagonal = y > 0 && x > 0 ? cluster_by_offset(index - width_ - 1) : nullptr;
                if ( diagonal_ajacency && cluster_diagonal && cluster_diagonal->color == color )
                {
                    bitmap_[index] = cluster_diagonal->id;
                    cluster_diagonal->size += 1;
                    old_clust = cluster_diagonal;
                    old_clust->index_end = index;

                }
                // Create a new cluster
                else
                {
                    old_clust = add_cluster(color);
                    old_clust->index_start = old_clust->index_end = index;
                    bitmap_[index] = old_clust->id;
                }
            }
            // Neighbour to the left
            else if ( cluster_left )
            {
                bitmap_[index] = cluster_left->id;
                cluster_left->size += 1;
                old_clust->index_end = index;

            }
            // Neighbour above
            else if ( cluster_up )
            {
                bitmap_[index] = cluster_up->id;
                cluster_up->size += 1;
                old_clust = cluster_up;
                old_clust->index_end = index;
            }
        }
    }
    normalize();
    clusters_.shrink_to_fit();
}


glaxnimate::trace::SegmentedImage glaxnimate::trace::segment(const QImage& image, bool diagonal_ajacency)
{
    QImage conv_image = image;
    if ( conv_image.format() != QImage::Format_ARGB32 )
        conv_image.convertTo(QImage::Format_ARGB32);
    auto pixels = reinterpret_cast<const quint32*>(conv_image.constBits());

    SegmentedImage segmented(image.width(), image.height());
    segmented.segment(pixels, diagonal_ajacency);
    return segmented;
}

void glaxnimate::trace::SegmentedImage::quantize(const std::vector<QRgb>& colors)
{
    auto min_id = next_id;

    for ( auto & cluster : *this )
        cluster.merge_target = min_id + closest_match(cluster.color, colors);

    for ( auto color : colors )
        add_cluster(color, 0);

    normalize();
}

void glaxnimate::trace::SegmentedImage::dilate(Cluster* source, int protect_size)
{
    std::map<Cluster::id_type, int> subtract;

    for ( std::size_t i = source->index_start; i <= source->index_end; i++ )
    {
        if ( bitmap_[i] == source->id )
        {
            int x = i % width_;
            int y = i / width_;
            for ( auto d : orthogonal_and_diagonal )
            {
                int fy = y + d.first;
                int fx = x + d.second;
                if ( fx < 0 || fy < 0 || fx >= width_ || fy >= height_ )
                    continue;

                auto& pix = bitmap_[fx + fy * width_];
                if ( pix > 0 && pix != source->id && (protect_size < 0 || cluster(pix)->size < protect_size) )
                {
                    subtract[pix] += 1;
                    pix = -source->id;
                }
            }
        }
    }

    for ( auto& pix : bitmap_ )
        if ( pix < 0 )
            pix = source->id;

    for ( const auto& sub : subtract )
    {
        auto sub_cluster = cluster(sub.first);
        sub_cluster->size -= sub.second;
        source->size += sub.second;
    }
}

int glaxnimate::trace::SegmentedImage::perimeter(glaxnimate::trace::Cluster* source) const
{
    int perimeter = 0;

    for ( std::size_t i = source->index_start; i <= source->index_end; i++ )
    {
        if ( bitmap_[i] == source->id )
        {
            int x = i % width_;
            int y = i / width_;
            for ( auto d : orthogonal )
            {
                if ( cluster_id(x + d.second, y + d.first) != source->id )
                {
                    perimeter += 1;
                    break;
                }
            }
        }
    }

    return perimeter;
}

std::vector<glaxnimate::trace::Cluster::id_type> glaxnimate::trace::SegmentedImage::neighbours(Cluster* cluster) const
{
    std::unordered_set<Cluster::id_type> neighbours;
    for ( auto i = cluster->index_start; i <= cluster->index_end; i++ )
    {
        if ( bitmap_[i] == cluster->id )
        {
            int x = i % width_;
            int y = i / width_;
            for ( auto d : orthogonal )
            {
                auto neigh = cluster_id(x + d.second, y + d.first);
                if ( neigh != cluster->id && neigh != Cluster::null_id )
                    neighbours.insert(neigh);
            }
        }
    }

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
