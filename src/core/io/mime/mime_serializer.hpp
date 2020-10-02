#pragma once

#include <memory>

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QMimeData>

#include "app/log/log_line.hpp"

namespace model {
    class Document;
    class DocumentNode;
} // namespace model

namespace io::mime {

struct DeserializedData
{
    DeserializedData();
    DeserializedData(const DeserializedData&) = delete;
    DeserializedData(DeserializedData&&);
    DeserializedData& operator=(const DeserializedData&) = delete;
    DeserializedData& operator=(DeserializedData&&);
    ~DeserializedData();

    std::unique_ptr<model::Document> document;

    bool empty() const;
    void initialize_data();
};

class MimeSerializer
{
public:
    virtual ~MimeSerializer() = default;

    virtual QString slug() const = 0;
    virtual QString name() const = 0;
    virtual QStringList mime_types() const = 0;

    virtual QByteArray serialize(const std::vector<model::DocumentNode*>& objects) const = 0;

    virtual io::mime::DeserializedData deserialize(const QByteArray& data) const;

    virtual bool can_deserialize() const = 0;

    virtual void to_mime_data(QMimeData& out, const std::vector<model::DocumentNode*>& objects) const
    {
        QByteArray data = serialize(objects);
        for ( const QString& mime : mime_types() )
            out.setData(mime, data);
    }

    io::mime::DeserializedData from_mime_data(const QMimeData& data) const;

protected:
    void message(const QString& message, app::log::Severity severity = app::log::Warning) const;

};

} // namespace io::mime
