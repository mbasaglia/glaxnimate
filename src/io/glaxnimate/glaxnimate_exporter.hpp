#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include "io/exporter.hpp"

namespace io::glaxnimate {


class GlaxnimateExporter : public ImportExportConcrete<GlaxnimateExporter, Exporter>
{
    Q_OBJECT

public:
    static constexpr const int format_version = 1;

    bool process(QIODevice& file, const QString&,
                 model::Document* document, const QVariantMap&) const override
    {
        return file.write(to_json(document).toJson(QJsonDocument::Indented));
    }
    QString name() const override { return tr("Glaxnimate Animation"); }
    QStringList extensions() const override { return {"glaxnim"}; }
    SettingList settings() const override
    {
        return {};
    }

    static QJsonDocument to_json(model::Document* document);
    static QJsonObject to_json(model::Object* object);
    static QJsonValue to_json(model::BaseProperty* property);
    static QJsonValue to_json(const QVariant& value);
    static QJsonValue to_json(const QVariant& value, model::PropertyTraits traits);
    static QJsonObject format_metadata();

    static GlaxnimateExporter* registered() { return autoreg.registered; }

private:
    static Autoreg autoreg;
};


} // namespace io::glaxnimate
