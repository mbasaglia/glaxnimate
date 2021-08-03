#pragma once

#include "io/base.hpp"
#include "io/io_registry.hpp"

namespace glaxnimate::io::spine {


class SpineFormat : public ImportExport
{
    Q_OBJECT

public:
    QString slug() const override { return "spine"; }
    QString name() const override { return tr("Spine JSON"); }
    QStringList extensions() const override { return {"json"}; }
    bool ignore_extension() const override { return true; }
    bool can_save() const override { return true; }
    bool can_open() const override { return true; }

private:
    bool on_save(QIODevice& file, const QString& filename,
                 model::Document* document, const QVariantMap& setting_values) override;

    bool on_open(QIODevice& file, const QString& filename,
                 model::Document* document, const QVariantMap& setting_values) override;

private:
    static Autoreg<SpineFormat> autoreg;
};


} // namespace glaxnimate::io::lottie

