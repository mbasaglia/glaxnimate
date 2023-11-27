/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QCborMap>
#include "io/base.hpp"
#include "io/io_registry.hpp"

namespace glaxnimate::io::lottie {


class LottieFormat : public ImportExport
{
    Q_OBJECT

public:
    QString slug() const override { return "lottie"_qs; }
    QString name() const override { return tr("Lottie Animation"); }
    QStringList extensions() const override { return {"json"_qs}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return true; }
    std::unique_ptr<app::settings::SettingsGroup> save_settings(model::Composition*) const override;

    QCborMap to_json(model::Composition* comp, bool strip = false, bool strip_raster = false, const QVariantMap& settings = {});
    bool load_json(const QByteArray& data, model::Document* document);

private:
    bool on_save(QIODevice& file, const QString& filename, model::Composition* comp, const QVariantMap& setting_values) override;

    bool on_open(QIODevice& file, const QString& filename,
                 model::Document* document, const QVariantMap& setting_values) override;
private:
    static Autoreg<LottieFormat> autoreg;
};


} // namespace glaxnimate::io::lottie
