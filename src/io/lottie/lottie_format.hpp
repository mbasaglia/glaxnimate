#pragma once

#include <QJsonDocument>
#include "io/base.hpp"

namespace io::lottie {


class LottieFormat : public ImportExport
{
    Q_OBJECT

public:
    QString name() const override { return tr("Lottie Animation"); }
    QStringList extensions() const override { return {"json"}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return true; }
    SettingList save_settings() const override
    {
        return {
            Setting("pretty", tr("Pretty"), tr("Pretty print the JSON"), false)
        };
    }

    static QJsonDocument to_json(model::Document* document);

private:
    bool on_save(QIODevice& file, const QString& filename,
                 model::Document* document, const QVariantMap& setting_values) override;
                 
    bool on_open(QIODevice& file, const QString& filename,
                 model::Document* document, const QVariantMap& setting_values) override;
private:
    static Autoreg<LottieFormat> autoreg;
};


} // namespace io::lottie
