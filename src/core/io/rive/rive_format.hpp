/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QJsonDocument>
#include "io/base.hpp"
#include "io/io_registry.hpp"

namespace glaxnimate::io::rive {


class RiveFormat : public ImportExport
{
    Q_OBJECT

public:
    static constexpr const int format_version = 7;

    QString slug() const override { return "rive"; }
    QString name() const override { return tr("Rive Animation"); }
    QStringList extensions() const override { return {"riv"}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return true; }

    static RiveFormat* instance() { return autoreg.registered; }

    QJsonDocument to_json(const QByteArray& binary_data);

protected:
    bool on_save(QIODevice& file, const QString&, model::Document* document, const QVariantMap&) override;
    bool on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&) override;

private:
    static Autoreg<RiveFormat> autoreg;
};


} // namespace glaxnimate::io::rive

