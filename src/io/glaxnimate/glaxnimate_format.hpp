#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include "io/base.hpp"

namespace io::glaxnimate {


class GlaxnimateFormat : public ImportExport
{
    Q_OBJECT

public:
    static constexpr const int format_version = 1;

    bool save(QIODevice& file, const QString&, model::Document* document, const QVariantMap&) override;
    bool open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&) override;
    QString name() const override { return tr("Glaxnimate Animation"); }
    QStringList extensions() const override { return {"glaxnim"}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return true; }

    static QJsonDocument to_json(model::Document* document);
    static QJsonObject to_json(model::Object* object);
    static QJsonValue to_json(model::BaseProperty* property);
    static QJsonValue to_json(const QVariant& value);
    static QJsonValue to_json(const QVariant& value, model::PropertyTraits traits);
    static QJsonObject format_metadata();

    static GlaxnimateFormat* registered() { return autoreg.registered; }

private:
    static Autoreg<GlaxnimateFormat> autoreg;

    class ImportState;

    void load_object(model::Object* target, const QJsonObject& object, ImportState& state);
    bool load_prop(model::BaseProperty* target, const QJsonValue& val, ImportState& state);
    QVariant load_prop_value(model::BaseProperty* target, const QJsonValue& val, ImportState& state);
    model::Object* create_object(const QString& type, ImportState& state);
};


} // namespace io::glaxnimate
