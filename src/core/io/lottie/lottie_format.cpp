#include "lottie_format.hpp"

#include "lottie_importer.hpp"
#include "lottie_exporter.hpp"

io::Autoreg<io::lottie::LottieFormat> io::lottie::LottieFormat::autoreg;

bool io::lottie::LottieFormat::on_save(QIODevice& file, const QString&,
                                         model::Document* document, const QVariantMap& setting_values)
{
    file.write(cbor_write_json(to_json(document), !setting_values["pretty"].toBool()));
    return true;
}

QCborMap io::lottie::LottieFormat::to_json(model::Document* document, bool strip)
{
    detail::LottieExporterState exp(this, document, strip);
    return exp.to_json();
}

bool io::lottie::LottieFormat::load_json(const QByteArray& data, model::Document* document)
{
    QJsonDocument jdoc;

    try {
        jdoc = QJsonDocument::fromJson(data);
    } catch ( const QJsonParseError& err ) {
        emit error(tr("Could not parse JSON: %1").arg(err.errorString()));
        return false;
    }

    if ( !jdoc.isObject() )
    {
        emit error(tr("No JSON object found"));
        return false;
    }

    QJsonObject top_level = jdoc.object();

    detail::LottieImporterState imp{document, this};
    imp.load(top_level);
    return true;
}

bool io::lottie::LottieFormat::on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&)
{
    return load_json(file.readAll(), document);
}
