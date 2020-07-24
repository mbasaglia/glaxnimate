#include "lottie_format.hpp"

#include <QJsonObject>
#include <QJsonArray>

using namespace model;

io::Autoreg<io::lottie::LottieFormat> io::lottie::LottieFormat::autoreg;


class LottieExporterState
{
public:
    explicit LottieExporterState(model::Document* document)
        : document(document) {}

    QJsonObject to_json()
    {
        /// @todo make a system that preserves key order as that is needed for lottie android
        return convert_animation(document->animation());
    }

    QJsonObject convert_animation(Animation* animation)
    {
        layer_indices.clear();
        QJsonObject json = convert_object_basic(animation, fields["Animation"]);

        QJsonArray layers;
        for ( const auto& layer : animation->layers )
            layers.append(convert_layer(layer.get()));

        json["layers"] = layers;
        return json;
    }

    int layer_index(Layer* layer)
    {
        if ( !layer )
            return -1;
        if ( !layer_indices.contains(layer->uuid.get()) )
            layer_indices[layer->uuid.get()] = layer_indices.size();
        return layer_indices[layer->uuid.get()];
    }

    QJsonObject convert_layer(Layer* layer)
    {
        QVariantMap special;
        int parent_index = layer_index(layer->parent.get());
        if ( parent_index != -1 )
            special["parent"] = parent_index;
        QJsonObject json = convert_object_basic(layer, fields["Layer"]);
        json["ty"] = layer_types[layer->type_name()];
        json["ind"] = layer_index(layer);

        return json;
    }

    QJsonObject convert_object_basic(model::Object* obj, const QVector<QPair<QString, QString>>& fields, const QVariantMap& special = {})
    {
        QJsonObject json_obj;
        for ( const auto& name : fields )
        {
            if ( name.first.isEmpty() )
            {
                if ( special.contains(name.second) )
                    json_obj[name.second] = QJsonValue::fromVariant(special[name.second]);
            }
            else
            {
                json_obj[name.second] = QJsonValue::fromVariant(obj->get(name.first));
            }
        }

        return json_obj;
    }

    model::Document* document;
    QMap<QUuid, int> layer_indices;

    // static mapping data
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
            // ddd
            // hd
            {"type",        "ty"},
            {"name",        "nm"},
            {"",            "parent"},
            // stretch sr
            // transform ks
            // auto_orient ao
            {"in_point",    "ip"},
            {"out_point",   "op"},
            {"start_time",  "st"},
            // blend_mode
            // matte_mode
            {"",            "ind"},
            // css_class cl
            // layer_html_id ln
            // hasMasks
            // masksProperties
            // effects ef

        }},
    };
    QMap<QString, int> layer_types = {
        {"EmptyLayer", 3},
        {"ShapeLayer", 4}
    };
};

bool io::lottie::LottieFormat::on_save(QIODevice& file, const QString&,
                                         model::Document* document, const QVariantMap& setting_values)
{
    file.write(to_json(document).toJson(setting_values["pretty"].toBool() ? QJsonDocument::Indented : QJsonDocument::Compact));
    return true;
}

QJsonDocument io::lottie::LottieFormat::to_json(model::Document* document)
{
    LottieExporterState exp(document);
    return QJsonDocument(exp.to_json());
}
