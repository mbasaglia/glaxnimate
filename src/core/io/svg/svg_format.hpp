#pragma once

#include "io/base.hpp"
#include "io/io_registry.hpp"

namespace io::svg {


class SvgFormat : public ImportExport
{
    Q_OBJECT

public:
    QString name() const override { return tr("Scalable Vector Graphics"); }
    QStringList extensions() const override { return {"svg", "svgz"}; }
    bool can_save() const override { return false; }
    bool can_open() const override { return true; }

protected:
    bool on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&) override;

private:
    static Autoreg<SvgFormat> autoreg;
};


} // namespace io::svg

