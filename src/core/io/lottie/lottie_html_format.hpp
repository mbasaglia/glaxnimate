/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "io/base.hpp"

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

    static QByteArray html_head(ImportExport* ie, model::Composition* comp, const QString& extra);

private:
    bool on_save(QIODevice& file, const QString& filename,
                 model::Composition* comp, const QVariantMap& setting_values) override;
};


} // namespace glaxnimate::io::lottie

