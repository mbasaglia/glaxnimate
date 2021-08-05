#include "lottie_format.hpp"

#include "lottie_importer.hpp"
#include "lottie_exporter.hpp"

glaxnimate::io::JsonImporter::Autoreg<glaxnimate::io::lottie::LottieFormat> glaxnimate::io::lottie::LottieFormat::autoreg;

bool glaxnimate::io::lottie::LottieFormat::on_save(QIODevice& file, const QString&,
                                         model::Document* document, const QVariantMap& setting_values)
{
    file.write(cbor_write_json(to_json(document), !setting_values["pretty"].toBool()));
    return true;
}

QCborMap glaxnimate::io::lottie::LottieFormat::to_json(model::Document* document, bool strip, bool strip_raster)
{
    detail::LottieExporterState exp(this, document, strip, strip_raster);
    return exp.to_json();
}


bool glaxnimate::io::lottie::LottieFormat::can_load_object(const QJsonObject& jdoc)
{
    return jdoc.contains("w");
}

bool glaxnimate::io::lottie::LottieFormat::on_load_json_object(const QJsonObject& json, model::Document* document, const QVariantMap&, const QString& )
{

    detail::LottieImporterState imp{document, this};
    imp.load(json);
    return true;
}

