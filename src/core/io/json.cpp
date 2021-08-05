#include "json.hpp"

#include <QJsonObject>
#include <QJsonDocument>

bool glaxnimate::io::JsonImporter::on_open(QIODevice& file, const QString& filename, model::Document* document, const QVariantMap& settings)
{
    return load_json_data(file.readAll(), document, settings, filename);
}

bool glaxnimate::io::JsonImporter::load_json_data(const QByteArray& data, model::Document* document, const QVariantMap& settings, const QString& filename)
{
    QJsonDocument jdoc;

    try {
        jdoc = QJsonDocument::fromJson(data);
    } catch ( const QJsonParseError& err ) {
        emit error(tr("Could not parse JSON: %1").arg(err.errorString()));
        return false;
    }

    return load_json(jdoc, document, settings, filename);
}

bool glaxnimate::io::JsonImporter::load_json(const QJsonDocument& jdoc, model::Document* document, const QVariantMap& settings, const QString& filename)
{
    if ( !jdoc.isObject() )
    {
        emit error(tr("No JSON object found"));
        return false;
    }

    return on_load_json_object(jdoc.object(), document, settings, filename);
}

std::vector<glaxnimate::io::JsonImporter *> & glaxnimate::io::JsonImporter::registered_importers()
{
    static std::vector<JsonImporter *> importers;
    return importers;
}

bool glaxnimate::io::JsonDispatchImporter::can_load_object(const QJsonObject&)
{
    return false;
}

bool glaxnimate::io::JsonDispatchImporter::on_load_json_object(const QJsonObject& jdoc, model::Document* document, const QVariantMap& settings, const QString& filename)
{
    for ( const auto& item : registered_importers() )
        if ( item->can_load_object(jdoc) )
        {
            connect(item, &ImportExport::message, this, &ImportExport::message);
            connect(item, &ImportExport::progress_max_changed, this, &ImportExport::progress_max_changed);
            connect(item, &ImportExport::progress, this, &ImportExport::progress);
            connect(item, &ImportExport::completed, this, &ImportExport::completed);
            bool ok = item->on_load_json_object(jdoc, document, settings, filename);
            disconnect(item, nullptr, this, nullptr);
            return ok;
        }
    return false;
}

glaxnimate::io::Autoreg<glaxnimate::io::JsonDispatchImporter> glaxnimate::io::JsonDispatchImporter::autoreg;

