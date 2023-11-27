/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once


#include "io/glaxnimate/glaxnimate_mime.hpp"

namespace glaxnimate::io::mime {

class JsonMime : public io::mime::MimeSerializer
{
public:
    QString slug() const override { return "json"_qs; }
    QString name() const override { return QObject::tr("JSON"); }
    QStringList mime_types() const override { return {"application/json"_qs, "text/plain"_qs}; }

    QByteArray serialize(const std::vector<model::DocumentNode*>& selection) const override
    {
        QJsonDocument json = io::glaxnimate::GlaxnimateMime::serialize_json(selection);
        return json.toJson(QJsonDocument::Indented);
    }

    bool can_deserialize() const override { return false; }

private:
    static Autoreg<JsonMime> autoreg;
};

} // namespace glaxnimate::io::mime
