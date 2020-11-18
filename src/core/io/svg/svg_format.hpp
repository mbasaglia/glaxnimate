#pragma once

#include "io/base.hpp"
#include "io/io_registry.hpp"

namespace io::svg {


class SvgFormat : public ImportExport
{
    Q_OBJECT

public:
    QString slug() const override { return "svg"; }
    QString name() const override { return tr("SVG"); }
    QStringList extensions() const override { return {"svg", "svgz"}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return true; }
    SettingList save_settings() const override;

protected:
    bool on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&) override;
    bool on_save(QIODevice & file, const QString & filename, model::Document * document, const QVariantMap & setting_values) override;

private:
    static Autoreg<SvgFormat> autoreg;
};


} // namespace io::svg

