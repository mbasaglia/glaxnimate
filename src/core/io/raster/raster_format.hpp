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
    QString slug() const override { return "image"; }
    QString name() const override { return tr("Raster Image"); }
    QStringList extensions() const override;
    bool can_save() const override { return false; }
    bool can_open() const override { return true; }

protected:
    bool on_open(QIODevice& dev, const QString&, model::Document* document, const QVariantMap&) override;

private:
    static Autoreg<RasterFormat> autoreg;
};


} // namespace io::raster

