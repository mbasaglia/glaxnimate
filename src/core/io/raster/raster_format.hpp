#pragma once

#include <QImageReader>

#include "io/base.hpp"
#include "io/io_registry.hpp"
#include "model/shapes/image.hpp"
#include "model/assets/assets.hpp"

namespace glaxnimate::io::raster {


class RasterFormat : public ImportExport
{
    Q_OBJECT

public:
    QString slug() const override { return "raster"; }
    QString name() const override { return tr("Raster Image"); }
    QStringList extensions() const override;
    bool can_save() const override { return false; }
    bool can_open() const override { return true; }
    int priority() const override { return -1; }

protected:
    bool on_open(QIODevice& dev, const QString&, model::Document* document, const QVariantMap&) override;

private:
    static Autoreg<RasterFormat> autoreg;
};


} // namespace glaxnimate::io::raster

