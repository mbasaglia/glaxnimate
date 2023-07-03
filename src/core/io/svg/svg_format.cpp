/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "svg_format.hpp"

#include <QFileInfo>

#include "utils/gzip.hpp"
#include "svg_parser.hpp"
#include "parse_error.hpp"
#include "svg_renderer.hpp"
#include "model/assets/assets.hpp"

glaxnimate::io::Autoreg<glaxnimate::io::svg::SvgFormat> glaxnimate::io::svg::SvgFormat::autoreg;

bool glaxnimate::io::svg::SvgFormat::on_open(QIODevice& file, const QString& filename, model::Document* document, const QVariantMap& options)
{
    /// \todo layer mode setting
    SvgParser::GroupMode mode = SvgParser::Inkscape;

    auto on_error = [this](const QString& s){warning(s);};
    try
    {
        QSize forced_size = options["forced_size"].toSize();
        model::FrameTime default_time = options["default_time"].toFloat();

        auto default_asset_path = QFileInfo(filename).dir();

        if ( utils::gzip::is_compressed(file) )
        {
            utils::gzip::GzipStream decompressed(&file, on_error);
            decompressed.open(QIODevice::ReadOnly);
            SvgParser(&decompressed, mode, document, on_error, this, forced_size, default_time, default_asset_path).parse_to_document();
            return true;
        }

        SvgParser(&file, mode, document, on_error, this, forced_size, default_time, default_asset_path).parse_to_document();
        return true;

    }
    catch ( const SvgParseError& err )
    {
        error(err.formatted(QFileInfo(filename).baseName()));
        return false;
    }
}

std::unique_ptr<app::settings::SettingsGroup> glaxnimate::io::svg::SvgFormat::save_settings(model::Composition* comp) const
{
    CssFontType max = CssFontType::None;
    for ( const auto & font : comp->document()->assets()->fonts->values )
    {
        auto type = SvgRenderer::suggested_type(font.get());
        if ( type > max )
            max = type;
    }

    if ( max == CssFontType::None )
        return {};

    QVariantMap choices;
    if ( max >= CssFontType::Link )
        choices[tr("External Stylesheet")] = int(CssFontType::Link);
    if ( max >= CssFontType::FontFace )
        choices[tr("Font face with external url")] = int(CssFontType::FontFace);
    if ( max >= CssFontType::Embedded )
        choices[tr("Embedded data")] = int(CssFontType::Embedded);
    choices[tr("Ignore")] = int(CssFontType::None);

    return std::make_unique<app::settings::SettingsGroup>(app::settings::SettingList{
        app::settings::Setting("font_type", tr("External Fonts"), tr("How to include external font"),
                               app::settings::Setting::Int, int(qMin(max, CssFontType::FontFace)), choices)
    });
}

bool glaxnimate::io::svg::SvgFormat::on_save(QIODevice& file, const QString& filename, model::Composition* comp, const QVariantMap& options)
{
    auto on_error = [this](const QString& s){warning(s);};
    SvgRenderer rend(SMIL, CssFontType(options["font_type"].toInt()));
    rend.write_main(comp);
    if ( filename.endsWith(".svgz") || options.value("compressed", false).toBool() )
    {
        utils::gzip::GzipStream compressed(&file, on_error);
        compressed.open(QIODevice::WriteOnly);
        rend.write(&compressed, false);
    }
    else
    {
        rend.write(&file, true);
    }

    return true;
}

