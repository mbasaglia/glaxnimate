#pragma once

#include "io/exporter.hpp"

namespace io::lottie {


class LottieExporter : public ImportExportConcrete<LottieExporter, Exporter>
{
public:
    bool process(const QString& filename, model::Document* document) const override;
    QString name() const override { return "Lottie"; }
    QString description() const override { return "Lottie Animation"; }
    QStringList extensions() const override { return {".json"}; }
    OptionList options() const override { return {}; }

private:
    static Autoreg autoreg;
};

} // namespace io::lottie
