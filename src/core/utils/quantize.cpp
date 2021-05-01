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

std::vector<QRgb> utils::quantize::k_modes(const QImage& image, int k)
{
    auto sortme = color_frequencies(image);
    std::sort(sortme.begin(), sortme.end(), [](const ColorFrequency& a, const ColorFrequency& b){ return a.second > b.second; });

    std::vector<QRgb> out;
    if ( int(sortme.size()) < k )
        k = sortme.size();
    out.reserve(k);
    for ( int i = 0; i < qMin<int>(k, sortme.size()); i++ )
        out.push_back(sortme[i].first);

    return out;
}

