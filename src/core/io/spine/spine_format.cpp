#include "spine_format.hpp"

#include <QJsonDocument>

#include "spine_importer.hpp"

glaxnimate::io::Autoreg<glaxnimate::io::spine::SpineFormat> glaxnimate::io::spine::SpineFormat::autoreg;

bool glaxnimate::io::spine::SpineFormat::on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&)
{
    QJsonDocument jdoc;

    try {
        jdoc = QJsonDocument::fromJson(file.readAll());
    } catch ( const QJsonParseError& err ) {
        error(tr("Could not parse JSON: %1").arg(err.errorString()));
        return false;
    }

    if ( !jdoc.isObject() )
    {
        error(tr("No JSON object found"));
        return false;
    }

    QJsonObject top_level = jdoc.object();

    SpineImporter imp{document, this};
    imp.load_document(top_level);

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


