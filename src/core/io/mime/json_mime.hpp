#pragma once


#include "io/glaxnimate/glaxnimate_mime.hpp"

namespace io::mime {

class JsonMime : public io::mime::MimeSerializer
{
public:
    QString slug() const override { return "json"; }
    QString name() const override { return QObject::tr("JSON"); }
    QStringList mime_types() const override { return {"application/json", "text/plain"}; }

    QByteArray serialize(const std::vector<model::DocumentNode*>& selection) const override
    {
        QJsonDocument json = io::glaxnimate::GlaxnimateMime::serialize_json(selection);
        return json.toJson(QJsonDocument::Indented);
    }

    std::vector<std::unique_ptr<model::DocumentNode>> deserialize(
        const QByteArray&,
        model::Document*,
        model::Composition*
    ) const override { return {}; }

    bool can_deserialize() const override { return false; }

private:
    static Autoreg<JsonMime> autoreg;
};

} // namespace io::mime
