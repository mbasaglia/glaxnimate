#pragma once

#include <QJsonDocument>
#include "io/exporter.hpp"

namespace io::lottie {


class LottieExporter : public ImportExportConcrete<LottieExporter, Exporter>
{
public:
    bool process(QIODevice& file, const QString& filename,
                 model::Document* document, const QVariantMap& option_values) const override;
    QString name() const override { return tr("Lottie Animation"); }
    QStringList extensions() const override { return {"json"}; }
    OptionList options() const override
    {
        return {
            Option("pretty", tr("Pretty"), tr("Pretty print the JSON"), false)
        };
    }

    static QJsonDocument to_json(model::Document* document);

private:
    static Autoreg autoreg;
};


} // namespace io::lottie
