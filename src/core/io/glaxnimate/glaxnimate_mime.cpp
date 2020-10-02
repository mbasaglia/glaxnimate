#include "glaxnimate_mime.hpp"
#include "import_state.hpp"
#include "model/shapes/shape.hpp"
#include "model/defs/named_color.hpp"
#include "model/defs/bitmap.hpp"

io::Autoreg<io::glaxnimate::GlaxnimateMime> io::glaxnimate::GlaxnimateMime::autoreg;

QStringList io::glaxnimate::GlaxnimateMime::mime_types() const
{
    return {"application/vnd.glaxnimate.rawr+json"};
}

QJsonDocument io::glaxnimate::GlaxnimateMime::serialize_json(const std::vector<model::DocumentNode *>& objects)
{
    QJsonArray arr;
    for ( auto object : objects )
        arr.push_back(GlaxnimateFormat::to_json(object));
    return QJsonDocument(arr);
}

QByteArray io::glaxnimate::GlaxnimateMime::serialize(const std::vector<model::DocumentNode*>& objects) const
{
    return serialize_json(objects).toJson(QJsonDocument::Compact);
}

io::mime::DeserializedData io::glaxnimate::GlaxnimateMime::deserialize(
    const QByteArray& data, model::Document* owner_document
)  const
{
    QJsonDocument jdoc;

    try {
        jdoc = QJsonDocument::fromJson(data);
    } catch ( const QJsonParseError& err ) {
        message(GlaxnimateFormat::tr("Could not parse JSON: %1").arg(err.errorString()));
        return {};
    }

    if ( !jdoc.isArray() )
    {
        message(GlaxnimateFormat::tr("No JSON object found"));
        return {};
    }

    QJsonArray input_objects = jdoc.array();

    detail::ImportState state(nullptr);
    state.document = owner_document;

    io::mime::DeserializedData output;
    output.initialize_data();

    for ( const auto& json_val : input_objects )
    {
        if ( !json_val.isObject() )
            continue;

        QJsonObject json_object = json_val.toObject();
        auto obj = model::Factory::instance().build(json_object["__type__"].toString(), owner_document);
        if ( !obj )
            continue;

        if ( auto shape = qobject_cast<model::ShapeElement*>(obj) )
        {
            output.document->main()->shapes.emplace(shape);
        }
        else if ( auto composition = qobject_cast<model::MainComposition*>(obj) )
        {
            output.document->main()->assign_from(composition);
            delete composition;
        }
        else if ( auto color = qobject_cast<model::NamedColor*>(obj) )
        {
            output.document->defs()->colors.emplace(color);
        }
        else if ( auto bitmap = qobject_cast<model::Bitmap*>(obj) )
        {
            output.document->defs()->images.emplace(bitmap);
        }
        else
        {
            delete obj;
            continue;
        }

        state.load_object(obj, json_object);
    }

    state.resolve();
    return output;
}

