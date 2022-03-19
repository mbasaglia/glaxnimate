#pragma once

#include "glaxnimate/core/io/base.hpp"

namespace glaxnimate::io::lottie {


class LottieHtmlFormat : public ImportExport
{
    Q_OBJECT

public:
    QString slug() const override { return "lottie_html"; }
    QString name() const override { return tr("Lottie HTML Preview"); }
    QStringList extensions() const override { return {"html", "htm"}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return false; }

    static QByteArray html_head(ImportExport* ie, model::Document* document, const QString& extra);

private:
    bool on_save(QIODevice& file, const QString& filename,
                 model::Document* document, const QVariantMap& setting_values) override;
};


} // namespace glaxnimate::io::lottie

