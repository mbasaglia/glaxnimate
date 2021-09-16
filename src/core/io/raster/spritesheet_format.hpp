#pragma once

#include <QImageReader>

#include "io/base.hpp"
#include "io/io_registry.hpp"

namespace glaxnimate::io::raster {


class SpritesheetFormat : public ImportExport
{
    Q_OBJECT

public:
    QString slug() const override { return "spritesheet"; }
    QString name() const override { return tr("Sprite Sheet"); }
    QStringList extensions() const override;

    std::unique_ptr<app::settings::SettingsGroup> save_settings(model::Document*) const override;

    bool can_save() const override { return true; }
    bool can_open() const override { return false; }

protected:
    bool on_save(QIODevice & file, const QString & filename, model::Document * document, const QVariantMap & setting_values) override;

private:
    static Autoreg<SpritesheetFormat> autoreg;
};


} // namespace glaxnimate::io::raster


