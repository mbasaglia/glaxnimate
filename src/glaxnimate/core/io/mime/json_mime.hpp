#pragma once


#include "glaxnimate/core/io/glaxnimate/glaxnimate_mime.hpp"

namespace glaxnimate::io::mime {

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

    bool can_deserialize() const override { return false; }

private:
    static Autoreg<JsonMime> autoreg;
};

} // namespace glaxnimate::io::mime
