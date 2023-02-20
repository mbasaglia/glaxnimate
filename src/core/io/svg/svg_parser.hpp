/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>
#include <vector>
#include <exception>
#include <QIODevice>

#include "io/mime/mime_serializer.hpp"
#include "io/base.hpp"

namespace glaxnimate::model {
    class Composition;
    class Document;
    class DocumentNode;
    class Object;
} // namespace glaxnimate::model


namespace glaxnimate::io::svg {

class SvgParser
{
public:
    // How to parse <g> elements
    enum GroupMode
    {
        Groups,     ///< As group shapes
        Layers,     ///< As shape layers
        Inkscape,   ///< Follow inkscape:groupmode
    };

    /**
     * \throws SvgParseError on error
     */
    SvgParser(
        QIODevice* device,
        GroupMode group_mode,
        model::Document* document,
        const std::function<void(const QString&)>& on_warning = {},
        ImportExport* io = nullptr,
        QSize forced_size = {}
    );
    ~SvgParser();

    void parse_to_document();
    io::mime::DeserializedData parse_to_objects();

    class Private;
private:
    std::unique_ptr<Private> d;
};

/**
 * \brief Parses a CSS color string
 * \see https://www.w3.org/wiki/CSS/Properties/color
 */
QColor parse_color(const QString& color_str);

} // namespace glaxnimate::io::svg
