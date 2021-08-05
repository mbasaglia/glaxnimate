#pragma once

#include "io/json.hpp"

namespace glaxnimate::io::spine {


class SpineFormat : public JsonImporter
{
    Q_OBJECT

public:
    QString slug() const override { return "spine"; }
    QString name() const override { return tr("Spine JSON"); }
    bool can_open() const override { return true; }
    bool can_save() const override { return false; }

private:
    bool on_save(QIODevice& file, const QString& filename,
                 model::Document* document, const QVariantMap& setting_values) override;

    bool on_load_json_object(const QJsonObject& json, model::Document* document, const QVariantMap& settings, const QString& filename) override;
    bool can_load_object(const QJsonObject& json) override;

private:
    static Autoreg<SpineFormat> autoreg;
};


} // namespace glaxnimate::io::lottie

