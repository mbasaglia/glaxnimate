/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QJsonDocument>
#include <QByteArray>
#include "io/mime/mime_serializer.hpp"
#include "glaxnimate_format.hpp"

namespace glaxnimate::io::glaxnimate {


class GlaxnimateMime : public io::mime::MimeSerializer
{
public:
    QString slug() const override { return "glaxnimate"_qs; }
    QString name() const override { return GlaxnimateFormat::tr("Glaxnimate Animation"); }
    QStringList mime_types() const override;
    QByteArray serialize(const std::vector<model::DocumentNode*>& objects) const override;
    io::mime::DeserializedData deserialize(const QByteArray& data) const override;
    bool can_deserialize() const override { return true; }

    static QJsonDocument serialize_json(const std::vector<model::DocumentNode*>& objects);

private:
    static Autoreg<GlaxnimateMime> autoreg;
};


} // namespace glaxnimate::io::glaxnimate
