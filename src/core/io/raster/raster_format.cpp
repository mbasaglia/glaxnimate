#include "raster_format.hpp"
#include "io/raster/raster_mime.hpp"

io::Autoreg<io::raster::RasterMime> io::raster::RasterMime::autoreg;
io::Autoreg<io::raster::RasterFormat> io::raster::RasterFormat::autoreg;


QStringList io::raster::RasterFormat::extensions() const
{
    QStringList formats;
    for ( const auto& fmt : QImageReader::supportedImageFormats() )
        if ( fmt != "gif" && fmt != "webp" )
            formats << QString::fromUtf8(fmt);
    return formats;
}

bool io::raster::RasterFormat::on_open(QIODevice& dev, const QString&, model::Document* document, const QVariantMap&)
{
    auto bmp = document->defs()->images.insert(std::make_unique<model::Bitmap>(document));
    if ( auto file = qobject_cast<QFile*>(&dev) )
        bmp->filename.set(file->fileName());
    else
        bmp->data.set(dev.readAll());
    auto img = std::make_unique<model::Image>(document);
    img->image.set(bmp);
    QPointF p(bmp->pixmap().width() / 2.0, bmp->pixmap().height() / 2.0);
    img->transform->anchor_point.set(p);
    img->transform->position.set(p);
    document->main()->shapes.insert(std::move(img));
    document->main()->width.set(bmp->pixmap().width());
    document->main()->height.set(bmp->pixmap().height());
    return !bmp->pixmap().isNull();
}
