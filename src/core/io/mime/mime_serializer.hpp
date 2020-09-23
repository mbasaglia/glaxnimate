#pragma once

#include <memory>

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QMimeData>

#include "app/log/log_line.hpp"

namespace model {
    class DocumentNode;
    class Document;
    class Composition;
} // namespace model

namespace io::mime {

class MimeSerializer
{
public:
    virtual ~MimeSerializer() = default;

    virtual QString slug() const = 0;
    virtual QString name() const = 0;
    virtual QStringList mime_types() const = 0;

    virtual QByteArray serialize(const std::vector<model::DocumentNode*>& objects) const = 0;

    virtual std::vector<std::unique_ptr<model::DocumentNode>> deserialize(
        const QByteArray& data,
        model::Document* owner_document,
        model::Composition* owner_composition
    ) const = 0;

    virtual bool can_deserialize() const = 0;

    virtual void to_mime_data(QMimeData& out, const std::vector<model::DocumentNode*>& objects) const
    {
        QByteArray data = serialize(objects);
        for ( const QString& mime : mime_types() )
            out.setData(mime, data);
    }

    std::vector<std::unique_ptr<model::DocumentNode>> from_mime_data(
        const QMimeData& data,
        model::Document* owner_document,
        model::Composition* owner_composition
    ) const;

protected:
    void message(const QString& message, app::log::Severity severity = app::log::Warning) const;

};

} // namespace io::mime
