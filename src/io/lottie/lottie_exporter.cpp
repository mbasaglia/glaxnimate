#include "lottie_exporter.hpp"

#include <QJsonObject>
#include <QJsonArray>

using namespace model;

io::lottie::LottieExporter::Autoreg io::lottie::LottieExporter::autoreg;


class LottieExporterState
{
public:
    explicit LottieExporterState(model::Document* document)
        : document(document) {}

    QJsonObject to_json()
    {
        return convert(&document->animation());
    }

    QJsonObject convert(Composition* composition)
    {
        QJsonObject json = convert_object_basic(composition);
        QJsonArray layers;
        for ( const auto& layer : composition->layers )
            layers.append(convert(layer.get()));

        json["layers"] = layers;
        return json;
    }

    QJsonObject convert(Object* obj)
    {
        return convert_object_basic(obj);
    }

    QJsonObject convert_object_basic(model::Object* obj)
    {
        QJsonObject json_obj;
        for ( BaseProperty* prop : obj->properties() )
        {
            if ( !prop->traits().is_object() )
                json_obj[prop->name()] = QJsonValue::fromVariant(prop->value());
        }

        return json_obj;
    }

    model::Document* document;
};

bool io::lottie::LottieExporter::process(QIODevice& file, const QString&,
                                         model::Document* document, const QVariantMap& setting_values) const
{
    file.write(to_json(document).toJson(setting_values["pretty"].toBool() ? QJsonDocument::Indented : QJsonDocument::Compact));
    return true;
}

QJsonDocument io::lottie::LottieExporter::to_json(model::Document* document)
{
    LottieExporterState exp(document);
    return QJsonDocument(exp.to_json());
}
