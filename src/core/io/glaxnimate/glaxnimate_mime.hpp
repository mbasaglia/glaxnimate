#pragma once

#include <QJsonDocument>
#include <QByteArray>
#include "io/mime/mime_serializer.hpp"
#include "glaxnimate_format.hpp"

namespace io::glaxnimate {


class GlaxnimateMime : public io::mime::MimeSerializer
{
public:
    QString slug() const override { return "glaxnimate"; }
    QString name() const override { return GlaxnimateFormat::tr("Glaxnimate Animation"); }
    QStringList mime_types() const override;
    QByteArray serialize(const std::vector<model::DocumentNode*>& objects) const override;
    io::mime::DeserializedData deserialize(
        const QByteArray& data,
        model::Document* owner_document,
        model::Composition* owner_composition
    ) const override;
    bool can_deserialize() const override { return true; }

    static QJsonDocument serialize_json(const std::vector<model::DocumentNode*>& objects);

private:
    static Autoreg<GlaxnimateMime> autoreg;
};


} // namespace io::glaxnimate
