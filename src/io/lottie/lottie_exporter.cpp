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
        return convert_animation(&document->animation());
    }

    QJsonObject convert_animation(Animation* animation)
    {
        QJsonObject json = convert_object_basic(animation, fields["Animation"]);

        QJsonArray layers;
        for ( const auto& layer : animation->layers )
            layers.append(convert_layer(layer.get()));

        json["layers"] = layers;
        return json;
    }

    QJsonObject convert_layer(Layer* layer)
    {
        QJsonObject json = convert_object_basic(layer, fields["Layer"]);
        if ( layer->parent.get() )
            json["parent"] = QJsonValue(layer->parent.get()->index.get());
        return json;
    }

    QJsonObject convert_object_basic(model::Object* obj, const QVector<QPair<QString, QString>>& fields)
    {
        QJsonObject json_obj;
        for ( const auto& name : fields )
        {
            json_obj[name.second] = QJsonValue::fromVariant(obj->get(name.first));
        }

        return json_obj;
    }

    model::Document* document;
    QMap<QString, QVector<QPair<QString, QString>>> fields = {
        {"Animation", {
            // version
            {"name",        "nm"},
            {"frame_rate",  "fr"},
            {"in_point",    "ip"},
            {"out_point",   "op"},
            {"width",       "w"},
            {"height",      "h"},
            // ddd
            // assets
            // comps
            // fonts
            // chars
            // markers
            // motion_blur
        }},
        {"Layer", {
            {"name",        "nm"},
            // ddd
            // hd
            {"type",        "ty"},
            // parent
            // stretch
            // transform
            // auto_orient
            {"in_point",    "ip"},
            {"out_point",   "op"},
            {"start_time",  "st"},
            // blend_mode
            // matte_mode
            {"index",       "ind"},
            // css_class
            // layer_html_id
            // has_masks
            // masks
            // effects

        }},
    };
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
