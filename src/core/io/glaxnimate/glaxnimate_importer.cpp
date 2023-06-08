/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate_format.hpp"

#include "import_state.hpp"
#include "model/assets/assets.hpp"

using namespace glaxnimate;

bool io::glaxnimate::GlaxnimateFormat::on_open ( QIODevice& file, const QString&, model::Document* document, const QVariantMap& )
{
    QJsonDocument jdoc;

    try {
        jdoc = QJsonDocument::fromJson(file.readAll());
    } catch ( const QJsonParseError& err ) {
        error(tr("Could not parse JSON: %1").arg(err.errorString()));
        return false;
    }

    if ( !jdoc.isObject() )
    {
        error(tr("No JSON object found"));
        return false;
    }

    QJsonObject top_level = jdoc.object();

    int document_version = top_level["format"].toObject()["format_version"].toInt(0);
    if ( document_version > format_version )
        warning(tr("Opening a file from a newer version of Glaxnimate"));

    detail::ImportState state(this, document, document_version);
    state.load_document(top_level);

    if ( document->assets()->compositions->values.empty() )
    {
        document->assets()->compositions->values.insert(std::make_unique<model::Composition>(document));
        error(tr("Missing composition"));
        return false;
    }

    return true;
}
