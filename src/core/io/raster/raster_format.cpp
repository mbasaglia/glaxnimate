/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "raster_format.hpp"

#include <QFileInfo>

#include "io/raster/raster_mime.hpp"
#include "utils/trace_wrapper.hpp"

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

bool glaxnimate::io::raster::RasterFormat::on_open(QIODevice& dev, const QString& filename, model::Document* document, const QVariantMap& settings)
{
    auto main = document->assets()->add_comp_no_undo();

    main->animation->last_frame.set(main->fps.get());


    model::FrameTime default_time = settings["default_time"_qs].toFloat();
    main->animation->last_frame.set(default_time == 0 ? default_time : 180);

#ifndef WITHOUT_POTRACE
    if ( settings.value("trace"_qs, {}).toBool() )
    {
        QImageReader reader;
        reader.setDevice(&dev);
        QImage image = reader.read();
        if ( image.isNull() )
            return false;

        utils::trace::TraceWrapper trace(main, image, filename);
        std::vector<QRgb> colors;
        std::vector<utils::trace::TraceWrapper::TraceResult> result;
        auto preset = trace.preset_suggestion();
        trace.trace_preset(preset, 16, colors, result);
        trace.apply(result, preset == utils::trace::TraceWrapper::PixelPreset ? 0 : 1);

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
    if ( !filename.isEmpty() )
        img->name.set(QFileInfo(filename).baseName());
    img->transform->anchor_point.set(p);
    img->transform->position.set(p);
    main->shapes.insert(std::move(img));
    main->width.set(bmp->pixmap().width());
    main->height.set(bmp->pixmap().height());
    return !bmp->pixmap().isNull();
}
