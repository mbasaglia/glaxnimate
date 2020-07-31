#pragma once

#include <QJsonDocument>
#include "io/base.hpp"

namespace io::lottie {


class LottieHtmlFormat : public ImportExport
{
    Q_OBJECT

public:
    QString name() const override { return tr("Lottie HTML Preview"); }
    QStringList extensions() const override { return {"html", "htm"}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return false; }

    static LottieHtmlFormat* registered() { return autoreg.registered; }

private:
    bool on_save(QIODevice& file, const QString& filename,
                 model::Document* document, const QVariantMap& setting_values) override;

    static Autoreg<LottieHtmlFormat> autoreg;
};


} // namespace io::lottie

