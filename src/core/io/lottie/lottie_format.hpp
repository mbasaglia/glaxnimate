#pragma once

#include <QCborMap>
#include "io/json.hpp"

namespace glaxnimate::io::lottie {


class LottieFormat : public JsonImporter
{
    Q_OBJECT

public:
    QString slug() const override { return "lottie"; }
    QString name() const override { return tr("Lottie Animation"); }
    bool can_save() const override { return true; }
    SettingList save_settings() const override
    {
        return {
            Setting("pretty", tr("Pretty"), tr("Pretty print the JSON"), false)
        };
    }

    QCborMap to_json(model::Document* document, bool strip = false, bool strip_raster = false);

private:
    bool on_save(QIODevice& file, const QString& filename,
                 model::Document* document, const QVariantMap& setting_values) override;

    bool on_load_json_object(const QJsonObject& json, model::Document* document, const QVariantMap& settings, const QString& filename) override;
    bool can_load_object(const QJsonObject& json) override;

private:
    static Autoreg<LottieFormat> autoreg;
};


} // namespace glaxnimate::io::lottie
