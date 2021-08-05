#include "spine_format.hpp"

#include <QJsonDocument>

#include "spine_importer.hpp"

glaxnimate::io::JsonImporter::Autoreg<glaxnimate::io::spine::SpineFormat> glaxnimate::io::spine::SpineFormat::autoreg;

bool glaxnimate::io::spine::SpineFormat::on_load_json_object(const QJsonObject& json, model::Document* document, const QVariantMap&, const QString& filename)
{
    SpineImporter imp{document, QFileInfo(filename).dir(), this};
    imp.load_document(json);
    return true;
}

bool glaxnimate::io::spine::SpineFormat::on_save(QIODevice& file, const QString& filename, model::Document* document, const QVariantMap& setting_values)
{
    /// \todo
    Q_UNUSED(file);
    Q_UNUSED(filename);
    Q_UNUSED(document);
    Q_UNUSED(setting_values);
    return false;
}

bool glaxnimate::io::spine::SpineFormat::can_load_object(const QJsonObject& json)
{
    return json.contains("bones");
}
