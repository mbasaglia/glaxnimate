#pragma once

#include <QImageReader>

#include "io/base.hpp"
#include "io/io_registry.hpp"
#include "model/shapes/image.hpp"
#include "model/defs/defs.hpp"

namespace io::raster {


class RasterFormat : public ImportExport
{
    Q_OBJECT

public:
    QString name() const override { return tr("Raster Image"); }
    QStringList extensions() const override
    {
        QStringList formats;
        for ( const auto& fmt : QImageReader::supportedImageFormats() )
            formats << QString::fromUtf8(fmt);
        return formats;
    }
    bool can_save() const override { return false; }
    bool can_open() const override { return true; }

protected:
    bool on_open(QIODevice& dev, const QString&, model::Document* document, const QVariantMap&) override
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
        return !bmp->pixmap().isNull();
    }

private:
    static Autoreg<RasterFormat> autoreg;
};


} // namespace io::raster

