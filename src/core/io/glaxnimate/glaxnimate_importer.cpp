#include "glaxnimate_format.hpp"

#include "import_state.hpp"

bool io::glaxnimate::GlaxnimateFormat::on_open ( QIODevice& file, const QString&, model::Document* document, const QVariantMap& )
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

    /// @todo check / handle format version
    // int document_format_version = top_level["format"].toObject()["format_version"].toInt(0);

    document->metadata() = top_level["metadata"].toObject().toVariantMap();

    if ( !top_level["animation"].isObject() )
    {
        error(tr("Missing animation object"));
        return false;
    }

    detail::ImportState state(this);
    state.document = document;
    state.composition = document->main();
    state.load_object(document->defs(), top_level["defs"].toObject());
    state.load_object(document->main(), top_level["animation"].toObject());
    state.resolve();

    return true;
}
