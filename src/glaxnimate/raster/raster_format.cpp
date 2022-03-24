#include "raster_format.hpp"
#include "glaxnimate/raster/raster_mime.hpp"
#include "glaxnimate/trace/trace_wrapper.hpp"

glaxnimate::io::Autoreg<glaxnimate::io::raster::RasterMime> glaxnimate::io::raster::RasterMime::autoreg;
glaxnimate::io::Autoreg<glaxnimate::io::raster::RasterFormat> glaxnimate::io::raster::RasterFormat::autoreg;


QStringList glaxnimate::io::raster::RasterFormat::extensions() const
{
    QStringList formats;
    for ( const auto& fmt : QImageReader::supportedImageFormats() )
        if ( fmt != "gif" && fmt != "webp" && fmt != "svg" )
            formats << QString::fromUtf8(fmt);
    return formats;
}

bool glaxnimate::io::raster::RasterFormat::on_open(QIODevice& dev, const QString&, model::Document* document, const QVariantMap& settings)
{
#ifndef WITHOUT_POTRACE
    if ( settings.value("trace", {}).toBool() )
    {
        QImageReader reader;
        reader.setDevice(&dev);
        QImage image = reader.read();
        if ( image.isNull() )
            return false;

        QString name = "";
        if ( auto file = qobject_cast<QFile*>(&dev) )
            name = file->fileName();
        trace::TraceWrapper trace(document, image, name);
        std::vector<trace::TraceWrapper::TraceResult> result;
        auto preset = trace.preset_suggestion();
        trace.trace_preset(preset, 16, result);
        trace.apply(result, preset == trace::TraceWrapper::PixelPreset ? 0 : 1);
        document->main()->width.set(image.width());
        document->main()->height.set(image.height());
        return true;
    }
#endif

    auto bmp = document->assets()->images->values.insert(std::make_unique<model::Bitmap>(document));
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
