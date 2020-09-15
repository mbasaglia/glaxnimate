#include "glaxnimate_mime.hpp"
#include "import_state.hpp"

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

std::vector<std::unique_ptr<model::DocumentNode>> io::glaxnimate::GlaxnimateMime::deserialize(
    const QByteArray& data, model::Document* owner_document, model::Composition* owner_composition
)  const
{
    QJsonDocument jdoc;

    try {
        jdoc = QJsonDocument::fromJson(data);
    } catch ( const QJsonParseError& err ) {
        return {};
    }

    if ( !jdoc.isArray() )
        return {};

    QJsonArray input_objects = jdoc.array();

    detail::ImportState state(nullptr);
    state.document = owner_document;
    state.composition = owner_composition;

    std::vector<std::unique_ptr<model::DocumentNode>> output_objects;
    output_objects.reserve(input_objects.size());

    for ( const auto& json_val : input_objects )
    {
        if ( !json_val.isObject() )
            continue;

        QJsonObject json_object = json_val.toObject();
        auto obj = model::Factory::instance().make_any(json_object["__type__"].toString(), owner_document, owner_composition);
        if ( !obj )
            continue;

        auto docnode = qobject_cast<model::DocumentNode*>(obj);
        if ( !docnode )
        {
            delete obj;
            return {};
        }

        output_objects.emplace_back(docnode);
        state.load_object(output_objects.back().get(), json_object);
    }

    state.resolve();
    return output_objects;
}

