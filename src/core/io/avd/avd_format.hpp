#pragma once

#include "io/base.hpp"
#include "io/io_registry.hpp"

namespace glaxnimate::io::avd {


class AvdFormat : public ImportExport
{
    Q_OBJECT

public:
    QString slug() const override { return "avd"; }
    QString name() const override { return tr("Android Vector Drawable"); }
    QStringList extensions() const override { return {"xml"}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return true; }

protected:
    bool on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap& options) override;
    bool on_save(QIODevice & file, const QString & filename, model::Document * document, const QVariantMap & setting_values) override;

private:
    static Autoreg<AvdFormat> autoreg;
};


} // namespace glaxnimate::io::avd


