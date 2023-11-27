/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "io/base.hpp"


namespace glaxnimate::io::rive {

class RiveHtmlFormat : public ImportExport
{
    Q_OBJECT

public:
    QString slug() const override { return "rive_html"_qs; }
    QString name() const override { return tr("RIVE HTML Preview"); }
    QStringList extensions() const override { return {"html"_qs, "htm"_qs}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return false; }

private:
    bool on_save(QIODevice& file, const QString& filename,
                 model::Composition* comp, const QVariantMap& setting_values) override;
};

} // namespace glaxnimate::io::rive
