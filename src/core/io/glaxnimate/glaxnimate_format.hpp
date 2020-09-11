#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include "io/base.hpp"

namespace io::glaxnimate {


class GlaxnimateFormat : public ImportExport
{
    Q_OBJECT

public:
    static constexpr const int format_version = 1;

    QString name() const override { return tr("Glaxnimate Animation"); }
    // RAWR = Reasonable Animation at Whatever Resolution
    QStringList extensions() const override { return {"rawr"}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return true; }

    static QJsonDocument to_json(model::Document* document);
    static QJsonObject to_json(model::Object* object);
    static QJsonValue to_json(model::BaseProperty* property);
    static QJsonValue to_json(const QVariant& value);
    static QJsonValue to_json(const QVariant& value, model::PropertyTraits traits);
    static QJsonObject format_metadata();
    static GlaxnimateFormat* instance() { return autoreg.registered; }

    static QString mime_type();
    static QJsonDocument serialize_json(const std::vector<model::DocumentNode*>& objects);
    static QByteArray serialize(const std::vector<model::DocumentNode*>& objects, bool pretty=false);
    static std::vector<std::unique_ptr<model::DocumentNode>> deserialize(
        const QByteArray& data,
        model::Document* owner_document,
        model::Composition* owner_composition
    );

    Q_INVOKABLE QVariant serialize(model::Object* object);

protected:
    bool on_save(QIODevice& file, const QString&, model::Document* document, const QVariantMap&) override;
    bool on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&) override;

private:
    static Autoreg<GlaxnimateFormat> autoreg;
    class ImportState;
};


} // namespace io::glaxnimate
