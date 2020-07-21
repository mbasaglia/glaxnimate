#pragma once

#include <QJsonDocument>
#include "io/exporter.hpp"

namespace io::lottie {


class LottieExporter : public ImportExportConcrete<LottieExporter, Exporter>
{
    Q_OBJECT

public:
    bool process(QIODevice& file, const QString& filename,
                 model::Document* document, const QVariantMap& setting_values) const override;
    QString name() const override { return tr("Lottie Animation"); }
    QStringList extensions() const override { return {"json"}; }
    SettingList settings() const override
    {
        return {
            Setting("pretty", tr("Pretty"), tr("Pretty print the JSON"), false)
        };
    }

    static QJsonDocument to_json(model::Document* document);

private:
    static Autoreg autoreg;
};


} // namespace io::lottie
