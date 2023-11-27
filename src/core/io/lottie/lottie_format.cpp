/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "lottie_format.hpp"

#include "lottie_importer.hpp"
#include "lottie_exporter.hpp"

glaxnimate::io::Autoreg<glaxnimate::io::lottie::LottieFormat> glaxnimate::io::lottie::LottieFormat::autoreg;

bool glaxnimate::io::lottie::LottieFormat::on_save(QIODevice& file, const QString&,
                                                   model::Composition* comp, const QVariantMap& setting_values)
{
    file.write(cbor_write_json(to_json(comp, setting_values["strip"_qs].toBool(), false, setting_values), !setting_values["pretty"_qs].toBool()));
    return true;
}

QCborMap glaxnimate::io::lottie::LottieFormat::to_json(model::Composition* comp, bool strip, bool strip_raster, const QVariantMap& settings)
{
    detail::LottieExporterState exp(this, comp, strip, strip_raster, settings);
    return exp.to_json();
}

bool glaxnimate::io::lottie::LottieFormat::load_json(const QByteArray& data, model::Document* document)
{
    QJsonDocument jdoc;

    try {
        jdoc = QJsonDocument::fromJson(data);
    } catch ( const QJsonParseError& err ) {
        Q_EMIT error(tr("Could not parse JSON: %1").arg(err.errorString()));
        return false;
    }

    if ( !jdoc.isObject() )
    {
        Q_EMIT error(tr("No JSON object found"));
        return false;
    }

    QJsonObject top_level = jdoc.object();

    detail::LottieImporterState imp{document, this};
    imp.load(top_level);
    return true;
}

bool glaxnimate::io::lottie::LottieFormat::on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&)
{
    return load_json(file.readAll(), document);
}

std::unique_ptr<app::settings::SettingsGroup> glaxnimate::io::lottie::LottieFormat::save_settings(model::Composition*) const
{
    return std::make_unique<app::settings::SettingsGroup>(app::settings::SettingList{
        app::settings::Setting("pretty"_qs, tr("Pretty"), tr("Pretty print the JSON"), false),
        app::settings::Setting("strip"_qs, tr("Strip"), tr("Strip unused properties"), false),
        app::settings::Setting("auto_embed"_qs, tr("Embed Images"), tr("Automatically embed non-embedded images"), false),
        app::settings::Setting("old_kf"_qs, tr("Legacy Keyframes"), tr("Compatibility with lottie-web versions prior to 5.0.0"), false),
    });
}
