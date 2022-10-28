#pragma once

#include "io/base.hpp"


namespace glaxnimate::io::rive {

class RiveHtmlFormat : public ImportExport
{
    Q_OBJECT

public:
    QString slug() const override { return "rive_html"; }
    QString name() const override { return tr("RIVE HTML Preview"); }
    QStringList extensions() const override { return {"html", "htm"}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return false; }

private:
    bool on_save(QIODevice& file, const QString& filename,
                 model::Document* document, const QVariantMap& setting_values) override;
};

} // namespace glaxnimate::io::rive
