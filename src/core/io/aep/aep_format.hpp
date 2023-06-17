/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "io/base.hpp"
#include "io/io_registry.hpp"

namespace glaxnimate::io::aep {

struct RiffChunk;

class AepFormat : public ImportExport
{
    Q_OBJECT

public:
    QString slug() const override { return "aep"; }
    QString name() const override { return tr("Adobe After Effects Project"); }
    QStringList extensions() const override { return {"aep"}; }
    bool can_save() const override { return false; }
    bool can_open() const override { return true; }

protected:
    bool on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap& options) override;

    bool riff_to_document(const RiffChunk& chunk, model::Document* document, const QString& filename);

private:
    static Autoreg<AepFormat> autoreg;
};


class AepxFormat : public AepFormat
{
    Q_OBJECT

public:
    QString slug() const override { return "aepx"; }
    QString name() const override { return tr("Adobe After Effects Project XML"); }
    QStringList extensions() const override { return {"aepx"}; }
    bool can_save() const override { return false; }
    bool can_open() const override { return true; }

protected:
    bool on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap& options) override;

private:
    static Autoreg<AepxFormat> autoreg;
};

} // namespace glaxnimate::io::aep



